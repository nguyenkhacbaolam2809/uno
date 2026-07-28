#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "card.h"
#include "player.h"
#include "game_engine.h"
#include "packets.h"
#include "tcp_buffer.h"
#include <string>
#include <vector>
#include <atomic>

struct SyncPlayer
{
    std::string name;
    PlayerType type;
    BotDifficulty difficulty;
    std::vector<card> hand;
};

struct SyncState
{
    GameState gs;
    std::vector<SyncPlayer> players;
    int myPlayerId;
};

class NetworkClient
{
public:
    NetworkClient();
    ~NetworkClient();

    bool connect(const std::string & host, int port = DEFAULT_PORT);
    void disconnect();
    bool receiveSyncState(SyncState & state, int timeoutMs = 100);
    bool sendPlayCard(int cardIdx, const std::string & chosenColor, int playerId);
    bool sendDraw(int playerId);
    bool sendJumpIn(int cardIdx, int playerId);
    bool sendCallUno(int playerId);
    bool sendCatchUno(int targetId, int playerId);

    bool isConnected() const { return connected; }

private:
    int sock;
    int epollFd;
    bool connected;
    RecvBuffer recvBuf;
    SendBuffer sendBuf;

    bool sendPacket(unsigned char type, unsigned char playerId, const void * body, int bodyLen);
};

#endif
