#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include "card.h"
#include "player.h"
#include "game_engine.h"
#include "packets.h"
#include "tcp_buffer.h"
#include "network_socket.h"
#include <string>
#include <vector>
#include <memory>
#include <deque>
#include <atomic>

struct SyncPlayer
{
    std::string name;
    PlayerType type;
    BotDifficulty difficulty;
    std::vector<Card> hand;
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
    bool sendPlayCard(int cardIdx, unsigned char chosenColor, int playerId);
    bool sendDraw(int playerId);
    bool sendJumpIn(int cardIdx, int playerId);
    bool sendCallUno(int playerId);
    bool sendCatchUno(int targetId, int playerId);

    bool isConnected() const { return connected; }

private:
    std::unique_ptr<TcpSocket> m_sock;
    std::unique_ptr<SocketPoller> m_poller;
    bool connected;
    RecvBuffer m_recvBuf;
    std::deque<std::vector<char>> m_sendQueue;
    int m_sendOffset{0};

    bool sendPacket(unsigned char type, unsigned char playerId, const void * body, int bodyLen);
    void drainSendQueue();
};

#endif
