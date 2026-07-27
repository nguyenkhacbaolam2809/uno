#include "network_server.h"
#include "rules.h"
#include "bot_factory.h"
#include <iostream>
#include <cstring>
#include <sstream>
using namespace std;

CRITICAL_SECTION NetworkServer::clientsLock;

struct ThreadParam
{
    NetworkServer * server;
    int clientIdx;
};

NetworkServer::NetworkServer(const GameConfig & cfg, bool vietRules)
    : config(cfg), vietRules(vietRules), engine(cfg, vietRules),
      listenSocket(INVALID_SOCKET), clientCount(0), running(false)
{
    for (int i = 0; i < MAX_CLIENTS; i++)
        clientSockets[i] = INVALID_SOCKET;
}

NetworkServer::~NetworkServer()
{
    stop();
}

bool NetworkServer::initWinsock()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        cerr << "WSAStartup failed: " << result << endl;
        return false;
    }
    return true;
}

void NetworkServer::cleanupWinsock()
{
    WSACleanup();
}

bool NetworkServer::start(int port)
{
    if (!initWinsock())
        return false;

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        cleanupWinsock();
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(listenSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "Bind failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        cleanupWinsock();
        return false;
    }

    if (listen(listenSocket, MAX_CLIENTS) == SOCKET_ERROR)
    {
        cerr << "Listen failed: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        cleanupWinsock();
        return false;
    }

    running = true;
    InitializeCriticalSection(&clientsLock);
    cout << msg(config, 53) << port << endl;
    return true;
}

void NetworkServer::stop()
{
    running = false;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientSockets[i] != INVALID_SOCKET)
        {
            closesocket(clientSockets[i]);
            clientSockets[i] = INVALID_SOCKET;
        }
    }

    if (listenSocket != INVALID_SOCKET)
    {
        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET;
    }

    DeleteCriticalSection(&clientsLock);
    cleanupWinsock();
}

bool NetworkServer::waitForPlayers(int expectedPlayers)
{
    while (running && clientCount < expectedPlayers)
    {
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET newSock = accept(listenSocket, (sockaddr *)&clientAddr, &addrLen);

        if (newSock == INVALID_SOCKET)
        {
            if (running)
                cerr << "Accept failed: " << WSAGetLastError() << endl;
            continue;
        }

        EnterCriticalSection(&clientsLock);
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clientSockets[i] == INVALID_SOCKET)
            {
                clientSockets[i] = newSock;
                clientCount++;

                ThreadParam * param = new ThreadParam;
                param->server = this;
                param->clientIdx = i;
                HANDLE hThread = CreateThread(NULL, 0, clientThreadStatic, param, 0, NULL);
                if (hThread) CloseHandle(hThread);

                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
                cout << "Player " << clientCount << " connected (" << clientIP << ")" << endl;
                break;
            }
        }
        LeaveCriticalSection(&clientsLock);
    }
    return running;
}

DWORD WINAPI NetworkServer::clientThreadStatic(LPVOID param)
{
    ThreadParam * p = (ThreadParam *)param;
    NetworkServer * server = p->server;
    int idx = p->clientIdx;
    delete p;
    return server->clientReceiveThread(idx);
}

DWORD NetworkServer::clientReceiveThread(int clientIdx)
{
    char buffer[BUFFER_SIZE];

    while (running)
    {
        int bytes = recv(clientSockets[clientIdx], buffer, sizeof(PacketHeader), 0);
        if (bytes <= 0)
        {
            if (running)
            {
                EnterCriticalSection(&clientsLock);
                removeClient(clientIdx);
                LeaveCriticalSection(&clientsLock);
            }
            break;
        }

        PacketHeader * header = (PacketHeader *)buffer;
        int totalSize = sizeof(PacketHeader) + header->bodyLen;

        while (bytes < totalSize)
        {
            int more = recv(clientSockets[clientIdx], buffer + bytes, totalSize - bytes, 0);
            if (more <= 0) break;
            bytes += more;
        }

        if (bytes < totalSize) break;

        switch (header->type)
        {
            case PKT_PLAY_CARD:
            {
                PacketPlayCard * pkt = (PacketPlayCard *)(buffer + sizeof(PacketHeader));
                string colorNames[] = {"", "do", "xanh la", "xanh duong", "vang"};
                string chosenCol = colorNames[pkt->chosenColor];
                engine.playCard(header->playerId, pkt->cardIndex, chosenCol);
                broadcastSyncState();
                break;
            }
            case PKT_DRAW:
            {
                engine.drawCard(header->playerId);
                broadcastSyncState();
                break;
            }
            case PKT_JUMP_IN:
            {
                PacketJumpIn * pkt = (PacketJumpIn *)(buffer + sizeof(PacketHeader));
                engine.jumpIn(header->playerId, pkt->cardIndex);
                broadcastSyncState();
                break;
            }
            case PKT_CALL_UNO:
            {
                engine.callUno(header->playerId);
                break;
            }
            case PKT_CATCH_UNO:
            {
                PacketUno * pkt = (PacketUno *)(buffer + sizeof(PacketHeader));
                engine.catchUno(header->playerId, pkt->targetId);
                broadcastSyncState();
                break;
            }
            default:
                break;
        }

        if (engine.isGameOver())
        {
            broadcastSyncState();
            break;
        }
    }

    return 0;
}

void NetworkServer::removeClient(int idx)
{
    if (clientSockets[idx] != INVALID_SOCKET)
    {
        closesocket(clientSockets[idx]);
        clientSockets[idx] = INVALID_SOCKET;
        clientCount--;
    }
}

bool NetworkServer::sendPacket(SOCKET sock, const void * data, int len)
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

bool NetworkServer::broadcastPacket(const void * data, int len, SOCKET exclude)
{
    EnterCriticalSection(&clientsLock);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientSockets[i] != INVALID_SOCKET && clientSockets[i] != exclude)
            sendPacket(clientSockets[i], data, len);
    }
    LeaveCriticalSection(&clientsLock);
    return true;
}

bool NetworkServer::sendSyncState(SOCKET sock)
{
    GameState gs = engine.getState();
    gs.vietRules = vietRules;

    int playerDataSize = 0;
    for (int i = 0; i < engine.getPlayerCount(); i++)
    {
        const player * p = engine.getPlayer(i);
        playerDataSize += sizeof(int) + (int)p->getName().length() + sizeof(int) + sizeof(int) + sizeof(int);
        for (int j = 0; j < p->get_size(); j++)
            playerDataSize += sizeof(card);
    }

    int totalSize = sizeof(PacketHeader) + sizeof(GameState) + sizeof(int) + playerDataSize;
    char * buffer = new char[totalSize];

    PacketHeader * header = (PacketHeader *)buffer;
    header->type = PKT_SYNC_STATE;
    header->playerId = 0;
    header->bodyLen = totalSize - sizeof(PacketHeader);

    int offset = sizeof(PacketHeader);
    memcpy(buffer + offset, &gs, sizeof(GameState));
    offset += sizeof(GameState);

    int pCount = engine.getPlayerCount();
    memcpy(buffer + offset, &pCount, sizeof(int));
    offset += sizeof(int);

    for (int i = 0; i < pCount; i++)
    {
        const player * p = engine.getPlayer(i);
        string name = p->getName();
        int nameLen = (int)name.length();
        int cardCount = p->get_size();
        int pType = (int)p->getType();
        int pDiff = (int)p->getDifficulty();

        memcpy(buffer + offset, &nameLen, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, name.c_str(), nameLen);
        offset += nameLen;
        memcpy(buffer + offset, &cardCount, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &pType, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &pDiff, sizeof(int));
        offset += sizeof(int);

        for (int j = 0; j < cardCount; j++)
        {
            card c = p->peek(j);
            memcpy(buffer + offset, &c, sizeof(card));
            offset += sizeof(card);
        }
    }

    bool result = sendPacket(sock, buffer, totalSize);
    delete[] buffer;
    return result;
}

void NetworkServer::broadcastSyncState()
{
    GameState gs = engine.getState();
    gs.vietRules = vietRules;

    int playerDataSize = 0;
    for (int i = 0; i < engine.getPlayerCount(); i++)
    {
        const player * p = engine.getPlayer(i);
        playerDataSize += sizeof(int) + (int)p->getName().length() + sizeof(int) + sizeof(int) + sizeof(int);
        for (int j = 0; j < p->get_size(); j++)
            playerDataSize += sizeof(card);
    }

    int totalSize = sizeof(PacketHeader) + sizeof(GameState) + sizeof(int) + playerDataSize;
    char * buffer = new char[totalSize];

    PacketHeader * header = (PacketHeader *)buffer;
    header->type = PKT_SYNC_STATE;
    header->playerId = 0;
    header->bodyLen = totalSize - sizeof(PacketHeader);

    int offset = sizeof(PacketHeader);
    memcpy(buffer + offset, &gs, sizeof(GameState));
    offset += sizeof(GameState);

    int pCount = engine.getPlayerCount();
    memcpy(buffer + offset, &pCount, sizeof(int));
    offset += sizeof(int);

    for (int i = 0; i < pCount; i++)
    {
        const player * p = engine.getPlayer(i);
        string name = p->getName();
        int nameLen = (int)name.length();
        int cardCount = p->get_size();
        int pType = (int)p->getType();
        int pDiff = (int)p->getDifficulty();

        memcpy(buffer + offset, &nameLen, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, name.c_str(), nameLen);
        offset += nameLen;
        memcpy(buffer + offset, &cardCount, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &pType, sizeof(int));
        offset += sizeof(int);
        memcpy(buffer + offset, &pDiff, sizeof(int));
        offset += sizeof(int);

        for (int j = 0; j < cardCount; j++)
        {
            card c = p->peek(j);
            memcpy(buffer + offset, &c, sizeof(card));
            offset += sizeof(card);
        }
    }

    EnterCriticalSection(&clientsLock);
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (clientSockets[i] != INVALID_SOCKET)
            sendPacket(clientSockets[i], buffer, totalSize);
    }
    LeaveCriticalSection(&clientsLock);

    delete[] buffer;
}

void NetworkServer::runGameLoop()
{
    engine.init(MAX_CLIENTS);

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        string pname = "Player ";
        pname += to_string(i + 1);
        engine.addPlayer(pname, HUMAN, 0);
    }

    engine.start();
    broadcastSyncState();

    while (running && !engine.isGameOver())
    {
        Sleep(100);
    }

    if (engine.isGameOver())
    {
        broadcastSyncState();
        cout << engine.getPlayer(engine.getWinner())->getName()
             << msg(config, 23) << endl;
    }

    Sleep(2000);
}
