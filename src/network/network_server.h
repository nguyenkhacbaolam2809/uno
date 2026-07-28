#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "game_engine.h"
#include "packets.h"
#include "net_platform.h"
#include <string>
#include <vector>
#include <memory>
#include <map>

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
    struct ClientContext {
        socket_t fd;
        TcpReader reader;
        TcpWriter writer;
        bool connected;
        ClientContext() : fd(INVALID_SOCK), connected(false) {}
    };

    GameConfig config;
    bool vietRules;
    GameEngine engine;

    socket_t listenSocket;
    std::map<int, ClientContext> clients;
    int clientCount;
    bool running;

    std::unique_ptr<EventLoop> loop;

    bool initPlatform();
    void cleanupPlatform();
    void onAccept();
    void onRead(int clientId);
    void onWrite(int clientId);
    void onClose(int clientId);
    void broadcastSyncState();
    void removeClient(int clientId);
    int nextClientId();
};

#endif
