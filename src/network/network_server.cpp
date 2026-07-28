#include "network_server.h"
#include "rules.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <string>

NetworkServer::NetworkServer(const GameConfig & cfg, bool viet)
    : config(cfg), vietRules(viet), engine(cfg, viet),
      listenSocket(INVALID_SOCK), clientCount(0), running(false)
{
}

NetworkServer::~NetworkServer()
{
    stop();
}

bool NetworkServer::initPlatform()
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed" << std::endl;
        return false;
    }
#endif
    loop = createEventLoop();
    return true;
}

void NetworkServer::cleanupPlatform()
{
    loop.reset();
#ifdef _WIN32
    WSACleanup();
#endif
}

bool NetworkServer::start(int port)
{
    if (!initPlatform())
        return false;

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCK)
    {
        std::cerr << "Socket creation failed" << std::endl;
        cleanupPlatform();
        return false;
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

#ifdef _WIN32
    if (bind(listenSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCK_ERR)
#else
    int opt = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(listenSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
#endif
    {
        std::cerr << "Bind failed" << std::endl;
        closesocket(listenSocket);
        cleanupPlatform();
        return false;
    }

    if (listen(listenSocket, MAX_CLIENTS) == SOCK_ERR)
    {
        std::cerr << "Listen failed" << std::endl;
        closesocket(listenSocket);
        cleanupPlatform();
        return false;
    }

    setNonBlocking(listenSocket);
    loop->watch(listenSocket, IOEvent::ACCEPT);
    running = true;

    std::cout << msg(config, 53) << port << std::endl;
    return true;
}

void NetworkServer::stop()
{
    running = false;
    if (loop) loop->stop();

    for (auto & kv : clients)
    {
        if (kv.second.fd != INVALID_SOCK)
        {
            loop->remove(kv.second.fd);
            closesocket(kv.second.fd);
        }
    }
    clients.clear();
    clientCount = 0;

    if (listenSocket != INVALID_SOCK)
    {
        loop->remove(listenSocket);
        closesocket(listenSocket);
        listenSocket = INVALID_SOCK;
    }
    cleanupPlatform();
}

int NetworkServer::nextClientId()
{
    for (int id = 0; id < MAX_CLIENTS; id++)
        if (clients.find(id) == clients.end())
            return id;
    return -1;
}

bool NetworkServer::waitForPlayers(int expectedPlayers)
{
    if (!loop) return false;

    while (running && clientCount < expectedPlayers)
    {
        IOEventData events[16];
        int n = loop->dispatch(events, 16, 1000);
        if (n < 0) continue;

        for (int i = 0; i < n; i++)
        {
            if (events[i].type == IOEvent::ACCEPT)
                onAccept();
        }
    }
    return running;
}

void NetworkServer::onAccept()
{
    sockaddr_in clientAddr;
#ifdef _WIN32
    int addrLen = sizeof(clientAddr);
#else
    socklen_t addrLen = sizeof(clientAddr);
#endif
    socket_t newFd = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
    if (newFd == INVALID_SOCK)
        return;

    setNonBlocking(newFd);

    int id = nextClientId();
    if (id < 0)
    {
        closesocket(newFd);
        return;
    }

    ClientContext ctx;
    ctx.fd = newFd;
    ctx.connected = true;
    clients[id] = std::move(ctx);
    clientCount++;

    loop->watch(newFd, IOEvent::READ);
    std::cout << "Player " << clientCount << " connected (id=" << id << ")" << std::endl;
}

void NetworkServer::onRead(int clientId)
{
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    ClientContext & ctx = it->second;
    if (!ctx.reader.readPacket(ctx.fd))
    {
        onClose(clientId);
        return;
    }

    PacketType type = ctx.reader.packetType();
    unsigned char pid = ctx.reader.playerId();
    const char * body = ctx.reader.body();
    int bodyLen = ctx.reader.bodyLen();

    switch (type)
    {
        case PKT_PLAY_CARD:
        {
            if (bodyLen >= (int)sizeof(PacketPlayCard))
            {
                const PacketPlayCard * pkt = reinterpret_cast<const PacketPlayCard *>(body);
                if (pkt->chosenColor <= 4)
                {
                    const char * colorNames[] = {"", "do", "xanh la", "xanh duong", "vang"};
                    std::string chosenCol = colorNames[pkt->chosenColor];
                    engine.playCard(pid, pkt->cardIndex, chosenCol);
                    broadcastSyncState();
                }
            }
            break;
        }
        case PKT_DRAW:
        {
            engine.drawCard(pid);
            broadcastSyncState();
            break;
        }
        case PKT_JUMP_IN:
        {
            if (bodyLen >= (int)sizeof(PacketJumpIn))
            {
                const PacketJumpIn * pkt = reinterpret_cast<const PacketJumpIn *>(body);
                engine.jumpIn(pid, pkt->cardIndex);
                broadcastSyncState();
            }
            break;
        }
        case PKT_CALL_UNO:
            engine.callUno(pid);
            break;
        case PKT_CATCH_UNO:
        {
            if (bodyLen >= (int)sizeof(PacketUno))
            {
                const PacketUno * pkt = reinterpret_cast<const PacketUno *>(body);
                engine.catchUno(pid, pkt->targetId);
                broadcastSyncState();
            }
            break;
        }
        default:
            break;
    }

    if (engine.isGameOver())
    {
        broadcastSyncState();
        running = false;
    }
}

void NetworkServer::onWrite(int clientId)
{
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    ClientContext & ctx = it->second;
    int result = ctx.writer.sendPending(ctx.fd);
    if (result == SOCK_ERR)
    {
        onClose(clientId);
        return;
    }
    if (ctx.writer.isIdle())
        loop->unwatch(ctx.fd, IOEvent::WRITE);
}

void NetworkServer::onClose(int clientId)
{
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    ClientContext & ctx = it->second;
    if (ctx.fd != INVALID_SOCK)
    {
        loop->remove(ctx.fd);
        closesocket(ctx.fd);
    }
    clients.erase(clientId);
    clientCount--;
    std::cout << "Player " << clientId << " disconnected" << std::endl;
}

void NetworkServer::removeClient(int clientId)
{
    onClose(clientId);
}

void NetworkServer::broadcastSyncState()
{
    GameState gs = engine.getState();
    gs.vietRules = vietRules;

    int playerDataSize = 0;
    for (int i = 0; i < engine.getPlayerCount(); i++)
    {
        const player * p = engine.getPlayer(i);
        playerDataSize += (int)sizeof(int) + (int)p->getName().length()
                        + (int)sizeof(int) + (int)sizeof(int) + (int)sizeof(int);
        playerDataSize += p->get_size() * (int)sizeof(card);
    }

    int bodySize = (int)sizeof(int) + (int)sizeof(GameState) + (int)sizeof(int) + playerDataSize;

    for (auto & kv : clients)
    {
        ClientContext & ctx = kv.second;
        if (!ctx.connected) continue;

        int myPlayerId = kv.first;

        std::vector<char> buf(bodySize);
        int offset = 0;

        std::memcpy(buf.data() + offset, &myPlayerId, sizeof(int));
        offset += sizeof(int);
        std::memcpy(buf.data() + offset, &gs, sizeof(GameState));
        offset += sizeof(GameState);

        int pCount = engine.getPlayerCount();
        std::memcpy(buf.data() + offset, &pCount, sizeof(int));
        offset += sizeof(int);

        for (int i = 0; i < pCount; i++)
        {
            const player * p = engine.getPlayer(i);
            std::string name = p->getName();
            int nameLen = (int)name.length();
            int cardCount = p->get_size();
            int pType = (int)p->getType();
            int pDiff = (int)p->getDifficulty();

            std::memcpy(buf.data() + offset, &nameLen, sizeof(int));
            offset += sizeof(int);
            std::memcpy(buf.data() + offset, name.c_str(), nameLen);
            offset += nameLen;
            std::memcpy(buf.data() + offset, &cardCount, sizeof(int));
            offset += sizeof(int);
            std::memcpy(buf.data() + offset, &pType, sizeof(int));
            offset += sizeof(int);
            std::memcpy(buf.data() + offset, &pDiff, sizeof(int));
            offset += sizeof(int);

            for (int j = 0; j < cardCount; j++)
            {
                card c = p->peek(j);
                std::memcpy(buf.data() + offset, &c, sizeof(card));
                offset += (int)sizeof(card);
            }
        }

        ctx.writer.beginPacket(PKT_SYNC_STATE, 0, bodySize);
        ctx.writer.writeBody(buf.data(), bodySize);

        int result = ctx.writer.sendPending(ctx.fd);
        if (result == SOCK_ERR)
        {
            onClose(kv.first);
            continue;
        }
        if (!ctx.writer.isIdle())
            loop->watch(ctx.fd, IOEvent::WRITE);
    }
}

void NetworkServer::runGameLoop()
{
    int connectedPlayers = clientCount;
    if (connectedPlayers == 0) return;

    engine.init(connectedPlayers);
    for (int i = 0; i < connectedPlayers; i++)
    {
        std::string pname = "Player ";
        pname += std::to_string(i + 1);
        engine.addPlayer(pname, HUMAN, 0);
    }

    engine.start();
    broadcastSyncState();

    while (running && !engine.isGameOver())
    {
        IOEventData events[16];
        int n = loop->dispatch(events, 16, 100);
        if (n < 0) continue;

        for (int i = 0; i < n; i++)
        {
            if (events[i].type == IOEvent::ACCEPT)
                onAccept();
            else
            {
                int clientId = -1;
                for (auto & kv : clients)
                {
                    if (kv.second.fd == events[i].fd)
                    {
                        clientId = kv.first;
                        break;
                    }
                }
                if (clientId < 0) continue;

                switch (events[i].type)
                {
                    case IOEvent::READ:  onRead(clientId); break;
                    case IOEvent::WRITE: onWrite(clientId); break;
                    case IOEvent::CLOSE: onClose(clientId); break;
                    default: break;
                }
            }
        }
    }

    if (engine.isGameOver())
    {
        broadcastSyncState();
        int winner = engine.getWinner();
        if (winner >= 0)
            std::cout << engine.getPlayer(winner)->getName()
                      << msg(config, 23) << std::endl;
    }
}
