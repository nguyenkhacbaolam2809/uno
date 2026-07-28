#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include "game_engine.h"
#include "packets.h"
#include "tcp_buffer.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <sys/epoll.h>

class NetworkServer
{
public:
    NetworkServer(const GameConfig & cfg, bool viet);
    ~NetworkServer();

    bool start(int port = DEFAULT_PORT);
    void stop();
    bool waitForPlayers(int expectedPlayers);
    void runGameLoop();
    bool isRunning() const { return running; }

private:
    struct Client {
        int fd;
        RecvBuffer recvBuf;
        SendBuffer sendBuf;
        bool connected;
        Client() : fd(-1), connected(false) {}
    };

    GameConfig config;
    bool vietRules;
    GameEngine engine;

    int listenFd;
    int epollFd;
    std::map<int, Client> clients;
    int clientCount;
    bool running;

    bool setNonBlocking(int fd);
    void handleAccept();
    void handleRead(int clientId);
    void handleWrite(int clientId);
    void handleClose(int clientId);
    void broadcastSyncState();
    int nextClientId();
};

#endif
