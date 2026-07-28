#include "network_client.h"
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>

NetworkClient::NetworkClient() : sock(INVALID_SOCKET), connected(false) {}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::initWinsock()
{
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void NetworkClient::cleanupWinsock()
{
    WSACleanup();
}

bool NetworkClient::connect(const std::string & host, int port)
{
    if (!initWinsock())
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        cleanupWinsock();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    if (::connect(sock, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        sock = INVALID_SOCKET;
        cleanupWinsock();
        return false;
    }

    connected = true;
    return true;
}

void NetworkClient::disconnect()
{
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    connected = false;
    cleanupWinsock();
}

bool NetworkClient::sendRaw(const void * data, int len)
{
    if (sock == INVALID_SOCKET) return false;
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
    if (sock == INVALID_SOCKET) return false;
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
bool NetworkClient::receiveSyncState(SyncState & state, int timeoutMs)
{
    PacketHeader header;
    if (!readRaw(&header, sizeof(PacketHeader)))
        return false;

    if (header.type != PKT_SYNC_STATE)
        return false;

    char * body = new char[header.bodyLen];
    if (!readRaw(body, header.bodyLen))
    {
        delete[] body;
        return false;
    }

    int offset = 0;
    memcpy(&state.gs, body + offset, sizeof(GameState));
    offset += sizeof(GameState);

    int pCount;
    memcpy(&pCount, body + offset, sizeof(int));
    offset += sizeof(int);

    state.players.clear();
    for (int i = 0; i < pCount; i++)
    {
        SyncPlayer sp;
        int nameLen;
        memcpy(&nameLen, body + offset, sizeof(int));
        offset += sizeof(int);

        char * nameBuf = new char[nameLen + 1];
        memcpy(nameBuf, body + offset, nameLen);
        nameBuf[nameLen] = 0;
        sp.name = nameBuf;
        delete[] nameBuf;
        offset += nameLen;

        int cardCount, pType, pDiff;
        memcpy(&cardCount, body + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&pType, body + offset, sizeof(int));
        offset += sizeof(int);
        memcpy(&pDiff, body + offset, sizeof(int));
        offset += sizeof(int);

        sp.type = (PlayerType)pType;
        sp.difficulty = (BotDifficulty)pDiff;

        for (int j = 0; j < cardCount; j++)
        {
            card c;
            memcpy(&c, body + offset, sizeof(card));
            offset += sizeof(card);
            sp.hand.push_back(c);
        }

        state.players.push_back(sp);
    }

    delete[] body;
    return true;
}

bool NetworkClient::sendPlayCard(int cardIdx, const std::string & chosenColor, int playerId)
{
    char buffer[sizeof(PacketHeader) + sizeof(PacketPlayCard)];
    PacketHeader * header = (PacketHeader *)buffer;
    header->type = PKT_PLAY_CARD;
    header->playerId = (unsigned char)playerId;
    header->bodyLen = sizeof(PacketPlayCard);

    PacketPlayCard * body = (PacketPlayCard *)(buffer + sizeof(PacketHeader));
    body->cardIndex = (unsigned char)cardIdx;

    if (chosenColor == "do" || chosenColor == "red") body->chosenColor = 1;
    else if (chosenColor == "xanh la" || chosenColor == "green") body->chosenColor = 2;
    else if (chosenColor == "xanh duong" || chosenColor == "blue") body->chosenColor = 3;
    else if (chosenColor == "vang" || chosenColor == "yellow") body->chosenColor = 4;
    else body->chosenColor = 0;

    return sendRaw(buffer, sizeof(buffer));
}

bool NetworkClient::sendDraw(int playerId)
{
    PacketHeader header;
    header.type = PKT_DRAW;
    header.playerId = (unsigned char)playerId;
    header.bodyLen = 0;
    return sendRaw(&header, sizeof(PacketHeader));
}

bool NetworkClient::sendJumpIn(int cardIdx, int playerId)
{
    char buffer[sizeof(PacketHeader) + sizeof(PacketJumpIn)];
    PacketHeader * header = (PacketHeader *)buffer;
    header->type = PKT_JUMP_IN;
    header->playerId = (unsigned char)playerId;
    header->bodyLen = sizeof(PacketJumpIn);

    PacketJumpIn * body = (PacketJumpIn *)(buffer + sizeof(PacketHeader));
    body->cardIndex = (unsigned char)cardIdx;
    return sendRaw(buffer, sizeof(buffer));
}

bool NetworkClient::sendCallUno(int playerId)
{
    PacketHeader header;
    header.type = PKT_CALL_UNO;
    header.playerId = (unsigned char)playerId;
    header.bodyLen = 0;
    return sendRaw(&header, sizeof(PacketHeader));
}

bool NetworkClient::sendCatchUno(int targetId, int playerId)
{
    char buffer[sizeof(PacketHeader) + sizeof(PacketUno)];
    PacketHeader * header = (PacketHeader *)buffer;
    header->type = PKT_CATCH_UNO;
    header->playerId = (unsigned char)playerId;
    header->bodyLen = sizeof(PacketUno);

    PacketUno * body = (PacketUno *)(buffer + sizeof(PacketHeader));
    body->targetId = (unsigned char)targetId;
    return sendRaw(buffer, sizeof(buffer));
}
