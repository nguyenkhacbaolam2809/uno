#include "network_client.h"
#include "network_socket.h"
#include "logger.h"
#include <cstring>
#include <deque>

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

    m_sock->setNonBlocking(true);
    m_poller = std::make_unique<SocketPoller>();
    m_poller->add(m_sock->fd(), true, false);

    connected = true;
    LOG_INFO("Connected to %s:%d", host.c_str(), port);
    return true;
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
}

bool NetworkClient::sendPacket(unsigned char type, unsigned char playerId,
                                const void * body, int bodyLen)
{
    if (!connected || !m_sock) return false;

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

    return true;
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

    int n = m_poller->wait(timeoutMs);
    if (n <= 0) return false;

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
        if (ev != PollEvent::Readable) continue;

        int recvd = m_recvBuf.fill(m_sock->fd());
        if (recvd <= 0)
        {
            connected = false;
            return false;
        }
    }

    if (!m_recvBuf.hasPacket())
        return false;

    auto pkt = m_recvBuf.readPacket();
    if (pkt.type != PacketType::SyncState)
        return false;

    const char * body = pkt.body.data();
    int bodyLen = (int)pkt.body.size();
    int offset = 0;

    if (bodyLen < (int)sizeof(int)) return false;
    std::memcpy(&state.myPlayerId, body + offset, sizeof(int));
    offset += sizeof(int);

    if (bodyLen - offset < (int)sizeof(GameState)) return false;
    std::memcpy(&state.gs, body + offset, sizeof(GameState));
    offset += sizeof(GameState);

    int pCount;
    std::memcpy(&pCount, body + offset, sizeof(int));
    offset += sizeof(int);

    state.players.clear();
    for (int i = 0; i < pCount; i++)
    {
        SyncPlayer sp;
        int nameLen;
        std::memcpy(&nameLen, body + offset, sizeof(int));
        offset += sizeof(int);
        if (offset + nameLen > bodyLen) return false;
        sp.name = std::string(body + offset, nameLen);
        offset += nameLen;

        int cardCount, pType, pDiff;
        std::memcpy(&cardCount, body + offset, sizeof(int));
        offset += sizeof(int);
        std::memcpy(&pType, body + offset, sizeof(int));
        offset += sizeof(int);
        std::memcpy(&pDiff, body + offset, sizeof(int));
        offset += sizeof(int);

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

bool NetworkClient::sendJumpIn(int cardIdx, int playerId)
{
    PacketJumpIn pkt;
    pkt.cardIndex = (unsigned char)cardIdx;
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
