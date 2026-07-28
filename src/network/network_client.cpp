#include "network_client.h"
#include <iostream>
#include <cstring>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

NetworkClient::NetworkClient() : sock(INVALID_SOCK), connected(false) {}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::connect(const std::string & host, int port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCK)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    if (::connect(sock, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        sock = INVALID_SOCK;
        WSACleanup();
        return false;
    }

    connected = true;
    return true;
}

void NetworkClient::disconnect()
{
    if (sock != INVALID_SOCK)
    {
        closesocket(sock);
        sock = INVALID_SOCK;
    }
    connected = false;
    WSACleanup();
}

bool NetworkClient::sendRaw(const void * data, int len)
{
    if (sock == INVALID_SOCK) return false;
    const char * buf = (const char *)data;
    int total = 0;
    while (total < len)
    {
        int sent = send(sock, buf + total, len - total, 0);
        if (sent == SOCKET_ERROR) return false;
        total += sent;
    }
    return true;
}

bool NetworkClient::readRaw(void * buffer, int len)
{
    if (sock == INVALID_SOCK) return false;
    char * buf = (char *)buffer;
    int total = 0;
    while (total < len)
    {
        int got = recv(sock, buf + total, len - total, 0);
        if (got <= 0) return false;
        total += got;
    }
    return true;
}

bool NetworkClient::receiveSyncState(SyncState & state, int /*timeoutMs*/)
{
    if (!reader.readPacket(sock))
        return false;

    if (reader.packetType() != PKT_SYNC_STATE)
        return false;

    const char * body = reader.body();
    int bodyLen = reader.bodyLen();
    int offset = 0;

    int myId;
    if (bodyLen < (int)sizeof(int)) return false;
    std::memcpy(&myId, body + offset, sizeof(int));
    state.myPlayerId = myId;
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

        std::string nameStr(body + offset, nameLen);
        sp.name = nameStr;
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
            card c;
            std::memcpy(&c, body + offset, sizeof(card));
            offset += sizeof(card);
            sp.hand.push_back(c);
        }

        state.players.push_back(sp);
    }

    return true;
}

bool NetworkClient::sendPlayCard(int cardIdx, const std::string & chosenColor, int playerId)
{
    char body[sizeof(PacketPlayCard)];
    PacketPlayCard * pkt = reinterpret_cast<PacketPlayCard *>(body);
    pkt->cardIndex = (unsigned char)cardIdx;

    if (chosenColor == "do" || chosenColor == "red") pkt->chosenColor = 1;
    else if (chosenColor == "xanh la" || chosenColor == "green") pkt->chosenColor = 2;
    else if (chosenColor == "xanh duong" || chosenColor == "blue") pkt->chosenColor = 3;
    else if (chosenColor == "vang" || chosenColor == "yellow") pkt->chosenColor = 4;
    else pkt->chosenColor = 0;

    writer.beginPacket(PKT_PLAY_CARD, (unsigned char)playerId, sizeof(PacketPlayCard));
    writer.writeBody(body, sizeof(PacketPlayCard));
    return sendRaw(writer.writeBuf.data(), (int)writer.writeBuf.size());
}

bool NetworkClient::sendDraw(int playerId)
{
    writer.beginPacket(PKT_DRAW, (unsigned char)playerId, 0);
    return sendRaw(writer.writeBuf.data(), (int)writer.writeBuf.size());
}

bool NetworkClient::sendJumpIn(int cardIdx, int playerId)
{
    char body[sizeof(PacketJumpIn)];
    PacketJumpIn * pkt = reinterpret_cast<PacketJumpIn *>(body);
    pkt->cardIndex = (unsigned char)cardIdx;

    writer.beginPacket(PKT_JUMP_IN, (unsigned char)playerId, sizeof(PacketJumpIn));
    writer.writeBody(body, sizeof(PacketJumpIn));
    return sendRaw(writer.writeBuf.data(), (int)writer.writeBuf.size());
}

bool NetworkClient::sendCallUno(int playerId)
{
    writer.beginPacket(PKT_CALL_UNO, (unsigned char)playerId, 0);
    return sendRaw(writer.writeBuf.data(), (int)writer.writeBuf.size());
}

bool NetworkClient::sendCatchUno(int targetId, int playerId)
{
    char body[sizeof(PacketUno)];
    PacketUno * pkt = reinterpret_cast<PacketUno *>(body);
    pkt->targetId = (unsigned char)targetId;

    writer.beginPacket(PKT_CATCH_UNO, (unsigned char)playerId, sizeof(PacketUno));
    writer.writeBody(body, sizeof(PacketUno));
    return sendRaw(writer.writeBuf.data(), (int)writer.writeBuf.size());
}
