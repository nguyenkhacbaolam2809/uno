#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "card.h"
#include "player.h"
#include "game_engine.h"
#include "packets.h"
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

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
    bool receiveSyncState(SyncState & state, int timeoutMs = 5000);
    bool sendPlayCard(int cardIdx, const std::string & chosenColor, int playerId);
    bool sendDraw(int playerId);
    bool sendJumpIn(int cardIdx, int playerId);
    bool sendCallUno(int playerId);
    bool sendCatchUno(int targetId, int playerId);

    bool isConnected() const { return connected; }

private:
    SOCKET sock;
    bool connected;

    bool initWinsock();
    void cleanupWinsock();
    bool sendRaw(const void * data, int len);
    bool readRaw(void * buffer, int len);
};

#endif
