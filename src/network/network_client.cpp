#include "network_client.h"
#include "network_socket.h"
#include "logger.h"
#include <cstring>
#include <deque>
#include <cerrno>

NetworkClient::NetworkClient()
    : m_sock(nullptr), connected(false)
{
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::connect(const std::string & host, int port)
{
    disconnect();

    m_sock = std::make_unique<TcpSocket>();
    if (!m_sock->connect(host, port, 3000))
    {
        m_sock.reset();
        LOG_ERROR("Failed to connect to %s:%d", host.c_str(), port);
        return false;
    }

    if (!m_sock->setNonBlocking(true))
    {
        LOG_ERROR("Failed to make client socket non-blocking");
        disconnect();
        return false;
    }

    m_poller = std::make_unique<SocketPoller>();
    if (!m_poller->add(m_sock->fd(), true, false))
    {
        LOG_ERROR("Failed to register client socket with poller");
        disconnect();
        return false;
    }

    connected = true;
    LOG_INFO("Connected to %s:%d", host.c_str(), port);

    PacketVersion version{ static_cast<unsigned char>(PROTOCOL_VERSION) };
    sendPacket(static_cast<unsigned char>(PacketType::Heartbeat), 0,
               &version, sizeof(version));

    return connected;
}

void NetworkClient::disconnect()
{
    connected = false;
    m_poller.reset();
    if (m_sock)
    {
        m_sock->close();
        m_sock.reset();
    }
    m_sendQueue.clear();
    m_sendOffset = 0;
    m_recvBuf = RecvBuffer();
}

bool NetworkClient::sendPacket(unsigned char type, unsigned char playerId,
                                const void * body, int bodyLen)
{
    if (!connected || !m_sock || bodyLen < 0 || bodyLen > UINT16_MAX) return false;

    uint32_t totalLen = 4 + 4 + bodyLen;
    std::vector<char> pkt(totalLen);

    std::memcpy(pkt.data(), &totalLen, 4);
    pkt[4] = static_cast<char>(type);
    pkt[5] = static_cast<char>(playerId);
    uint16_t bLen = static_cast<uint16_t>(bodyLen);
    std::memcpy(pkt.data() + 6, &bLen, 2);

    if (body && bodyLen > 0)
        std::memcpy(pkt.data() + 10, body, bodyLen);

    m_sendQueue.push_back(std::move(pkt));

    drainSendQueue();

    return connected;
}

void NetworkClient::drainSendQueue()
{
    while (!m_sendQueue.empty())
    {
        auto & pkt = m_sendQueue.front();
        int n = m_sock->send(pkt.data() + m_sendOffset, (int)pkt.size() - m_sendOffset);
        if (n > 0)
        {
            m_sendOffset += n;
            if (m_sendOffset >= (int)pkt.size())
            {
                m_sendQueue.pop_front();
                m_sendOffset = 0;
            }
        }
        else
        {
            if (n == 0)
            {
                connected = false;
                break;
            }
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            connected = false;
            break;
        }
    }

    if (m_poller && m_sock)
    {
        bool wantWrite = !m_sendQueue.empty();
        m_poller->modify(m_sock->fd(), true, wantWrite);
    }
}

bool NetworkClient::receiveSyncState(SyncState & state, int timeoutMs)
{
    if (!connected || !m_sock || !m_poller) return false;

    // Drain send queue first
    drainSendQueue();
    if (!connected) return false;

    auto parseBuffered = [&](bool & found) -> bool {
        found = false;
        while (m_recvBuf.hasPacket())
        {
            auto pkt = m_recvBuf.readPacket();
            if (pkt.type == PacketType::Heartbeat)
                continue;
            if (pkt.type != PacketType::SyncState)
                continue;

            found = true;
            const char * body = pkt.body.data();
            int bodyLen = (int)pkt.body.size();
            int offset = 0;

            auto readInt = [&](int & value) {
                if (bodyLen - offset < static_cast<int>(sizeof(value)))
                    return false;
                std::memcpy(&value, body + offset, sizeof(value));
                offset += static_cast<int>(sizeof(value));
                return true;
            };

            if (!readInt(state.myPlayerId) ||
                bodyLen - offset < static_cast<int>(sizeof(GameState)))
                return false;
            std::memcpy(&state.gs, body + offset, sizeof(GameState));
            offset += sizeof(GameState);

            int pCount;
            if (!readInt(pCount) || pCount < 0 || pCount > MAX_PLAYERS)
                return false;

            state.players.clear();
            for (int i = 0; i < pCount; i++)
            {
                SyncPlayer sp;
                int nameLen;
                if (!readInt(nameLen) || nameLen < 0 || nameLen > bodyLen - offset)
                    return false;
                sp.name = std::string(body + offset, nameLen);
                offset += nameLen;

                int cardCount, pType, pDiff;
                if (!readInt(cardCount) || !readInt(pType) || !readInt(pDiff) ||
                    cardCount < 0 || cardCount > DECK_SIZE ||
                    pType < 0 || pType > 1 ||
                    pDiff < 0 || pDiff > 2)
                    return false;

                sp.type = static_cast<PlayerType>(pType);
                sp.difficulty = static_cast<BotDifficulty>(pDiff);

                for (int j = 0; j < cardCount; j++)
                {
                    if (offset + (int)sizeof(Card) > bodyLen) return false;
                    Card c;
                    std::memcpy(&c, body + offset, sizeof(Card));
                    offset += (int)sizeof(Card);
                    sp.hand.push_back(c);
                }

                state.players.push_back(sp);
            }
            return true;
        }
        return false;
    };

    // A previous poll may have left more than one complete packet buffered.
    bool found = false;
    if (parseBuffered(found))
        return true;
    if (found)
        return false;

    int n = m_poller->wait(timeoutMs);
    if (n <= 0) return false;

    bool readable = false;
    for (int i = 0; i < n; i++)
    {
        socket_t fd = m_poller->getFd(i);
        if (fd != m_sock->fd()) continue;

        PollEvent ev = m_poller->getEvent(i);
        if (ev == PollEvent::Error || ev == PollEvent::Disconnected)
        {
            LOG_WARN("Server disconnected");
            connected = false;
            return false;
        }
        if (ev == PollEvent::Writable)
        {
            drainSendQueue();
            continue;
        }
        if (ev == PollEvent::Readable)
            readable = true;
    }

    if (!readable) return false;

    // Edge-triggered pollers only report the transition; drain until EAGAIN.
    for (;;)
    {
        int recvd = m_recvBuf.fill(m_sock->fd());
        if (recvd > 0)
            continue;
        if (recvd == 0)
        {
            connected = false;
            return false;
        }
        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            break;
        connected = false;
        return false;
    }

    if (parseBuffered(found))
        return true;
    return false;
}

bool NetworkClient::sendPlayCard(int cardIdx, unsigned char chosenColor, int playerId)
{
    PacketPlayCard pkt;
    pkt.cardIndex = (unsigned char)cardIdx;
    pkt.chosenColor = chosenColor;
    return sendPacket(static_cast<unsigned char>(PacketType::PlayCard),
                      (unsigned char)playerId, &pkt, sizeof(pkt));
}

bool NetworkClient::sendDraw(int playerId)
{
    return sendPacket(static_cast<unsigned char>(PacketType::Draw),
                      (unsigned char)playerId, nullptr, 0);
}

bool NetworkClient::sendJumpIn(int cardIdx, int playerId, unsigned char chosenColor)
{
    PacketJumpIn pkt;
    pkt.cardIndex = (unsigned char)cardIdx;
    pkt.chosenColor = chosenColor;
    return sendPacket(static_cast<unsigned char>(PacketType::JumpIn),
                      (unsigned char)playerId, &pkt, sizeof(pkt));
}

bool NetworkClient::sendCallUno(int playerId)
{
    return sendPacket(static_cast<unsigned char>(PacketType::CallUno),
                      (unsigned char)playerId, nullptr, 0);
}

bool NetworkClient::sendCatchUno(int targetId, int playerId)
{
    PacketUno pkt;
    pkt.targetId = (unsigned char)targetId;
    return sendPacket(static_cast<unsigned char>(PacketType::CatchUno),
                      (unsigned char)playerId, &pkt, sizeof(pkt));
}
