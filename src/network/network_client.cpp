#include "network_client.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <netdb.h>

NetworkClient::NetworkClient() : sock(-1), epollFd(-1), connected(false)
{
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::connect(const std::string & host, int port)
{
    sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sock < 0) return false;

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    struct hostent * he = gethostbyname(host.c_str());
    if (he)
        std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    else
        inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    ::connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    epollFd = epoll_create1(0);
    if (epollFd < 0) { close(sock); sock = -1; return false; }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
    ev.data.fd = sock;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &ev);

    struct epoll_event events[1];
    int ret = epoll_wait(epollFd, events, 1, 3000);
    if (ret <= 0)
    {
        close(sock); close(epollFd);
        sock = -1; epollFd = -1;
        return false;
    }

    int err = 0;
    socklen_t errLen = sizeof(err);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &errLen);
    if (err != 0)
    {
        close(sock); close(epollFd);
        sock = -1; epollFd = -1;
        return false;
    }

    connected = true;
    return true;
}

void NetworkClient::disconnect()
{
    connected = false;
    if (sock >= 0) { close(sock); sock = -1; }
    if (epollFd >= 0) { close(epollFd); epollFd = -1; }
}

bool NetworkClient::sendPacket(unsigned char type, unsigned char playerId,
                                const void * body, int bodyLen)
{
    if (!connected) return false;
    sendBuf.beginPacket(type, playerId, bodyLen);
    if (body && bodyLen > 0)
        sendBuf.writeBody(body, bodyLen);
    sendBuf.flush(sock);
    return true;
}

bool NetworkClient::receiveSyncState(SyncState & state, int timeoutMs)
{
    if (!connected) return false;

    struct epoll_event events[1];
    int ret = epoll_wait(epollFd, events, 1, timeoutMs);
    if (ret <= 0) return false;

    int n = recvBuf.fill(sock);
    if (n <= 0)
    {
        if (n == 0) connected = false;
        return false;
    }

    if (!recvBuf.hasPacket())
        return false;

    auto pkt = recvBuf.readPacket();
    if (pkt.type != PKT_SYNC_STATE)
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

        sp.type = (PlayerType)pType;
        sp.difficulty = (BotDifficulty)pDiff;

        for (int j = 0; j < cardCount; j++)
        {
            if (offset + (int)sizeof(card) > bodyLen) return false;
            card c;
            std::memcpy(&c, body + offset, sizeof(card));
            offset += (int)sizeof(card);
            sp.hand.push_back(c);
        }

        state.players.push_back(sp);
    }

    return true;
}

bool NetworkClient::sendPlayCard(int cardIdx, const std::string & chosenColor, int playerId)
{
    PacketPlayCard pkt;
    pkt.cardIndex = (unsigned char)cardIdx;

    if (chosenColor == "do" || chosenColor == "red") pkt.chosenColor = 1;
    else if (chosenColor == "xanh la" || chosenColor == "green") pkt.chosenColor = 2;
    else if (chosenColor == "xanh duong" || chosenColor == "blue") pkt.chosenColor = 3;
    else if (chosenColor == "vang" || chosenColor == "yellow") pkt.chosenColor = 4;
    else pkt.chosenColor = 0;

    return sendPacket(PKT_PLAY_CARD, (unsigned char)playerId, &pkt, sizeof(pkt));
}

bool NetworkClient::sendDraw(int playerId)
{
    return sendPacket(PKT_DRAW, (unsigned char)playerId, nullptr, 0);
}

bool NetworkClient::sendJumpIn(int cardIdx, int playerId)
{
    PacketJumpIn pkt;
    pkt.cardIndex = (unsigned char)cardIdx;
    return sendPacket(PKT_JUMP_IN, (unsigned char)playerId, &pkt, sizeof(pkt));
}

bool NetworkClient::sendCallUno(int playerId)
{
    return sendPacket(PKT_CALL_UNO, (unsigned char)playerId, nullptr, 0);
}

bool NetworkClient::sendCatchUno(int targetId, int playerId)
{
    PacketUno pkt;
    pkt.targetId = (unsigned char)targetId;
    return sendPacket(PKT_CATCH_UNO, (unsigned char)playerId, &pkt, sizeof(pkt));
}
