#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "game_engine.h"
#include "packets.h"
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

class NetworkServer
{
public:
    NetworkServer(const GameConfig & cfg, bool vietRules);
    ~NetworkServer();

    bool start(int port = DEFAULT_PORT);
    void stop();
    bool waitForPlayers(int expectedPlayers);
    void runGameLoop();
    bool isRunning() const { return running; }

private:
    GameConfig config;
    bool vietRules;
    GameEngine engine;

    SOCKET listenSocket;
    SOCKET clientSockets[MAX_CLIENTS];
    int clientCount;
    bool running;

    static CRITICAL_SECTION clientsLock;

    bool initWinsock();
    void cleanupWinsock();
    bool sendPacket(SOCKET sock, const void * data, int len);
    bool sendSyncState(SOCKET sock);
    bool broadcastPacket(const void * data, int len, SOCKET exclude = INVALID_SOCKET);
    void broadcastSyncState();
    void removeClient(int idx);
    DWORD clientReceiveThread(int clientIdx);

    static DWORD WINAPI clientThreadStatic(LPVOID param);
};

#endif
