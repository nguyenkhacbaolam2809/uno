#include "network_server.h"
#include "rules.h"
#include "logger.h"
#include <cstring>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

NetworkServer::NetworkServer(const GameConfig & cfg, bool viet)
    : config(cfg), engine(cfg, viet),
      listenFd(-1), epollFd(-1), clientCount(0), running(false)
{
}

NetworkServer::~NetworkServer()
{
    stop();
}

bool NetworkServer::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

bool NetworkServer::start(int port)
{
    listenFd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (listenFd < 0) return false;

    int opt = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    { close(listenFd); return false; }

    if (listen(listenFd, MAX_CLIENTS) < 0)
    { close(listenFd); return false; }

    epollFd = epoll_create1(0);
    if (epollFd < 0) { close(listenFd); return false; }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.u32 = 0;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &ev);

    running = true;
    LOG_INFO("Server started on port %d", port);
    return true;
}

void NetworkServer::stop()
{
    running = false;

    for (auto & kv : clients)
    {
        if (kv.second.fd >= 0)
        {
            epoll_ctl(epollFd, EPOLL_CTL_DEL, kv.second.fd, nullptr);
            close(kv.second.fd);
        }
    }
    clients.clear();
    clientCount = 0;

    if (listenFd >= 0)
    {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, listenFd, nullptr);
        close(listenFd);
        listenFd = -1;
    }

    if (epollFd >= 0)
    {
        close(epollFd);
        epollFd = -1;
    }
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
    struct epoll_event events[16];

    while (running && clientCount < expectedPlayers)
    {
        int n = epoll_wait(epollFd, events, 16, 1000);
        if (n < 0) continue;

        for (int i = 0; i < n; i++)
        {
            if (events[i].data.u32 == 0)
                handleAccept();
        }
    }
    return running;
}

void NetworkServer::handleAccept()
{
    sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    int newFd = accept(listenFd, (struct sockaddr *)&addr, &addrLen);
    if (newFd < 0) return;
    setNonBlocking(newFd);

    int id = nextClientId();
    if (id < 0) { close(newFd); return; }

    Client ctx;
    ctx.fd = newFd;
    ctx.connected = true;
    clients[id] = std::move(ctx);
    clientCount++;

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    ev.data.u32 = id + 1;
    epoll_ctl(epollFd, EPOLL_CTL_ADD, newFd, &ev);

    LOG_INFO("Player %d connected (id=%d)", clientCount, id);
}

void NetworkServer::handleRead(int clientId)
{
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    Client & ctx = it->second;
    int n = ctx.recvBuf.fill(ctx.fd);
    if (n <= 0) { handleClose(clientId); return; }

    while (ctx.recvBuf.hasPacket())
    {
        auto pkt = ctx.recvBuf.readPacket();

        if (pkt.type == PacketType::Heartbeat)
        {
            if (pkt.body.size() >= sizeof(PacketVersion))
            {
                const PacketVersion * pv = reinterpret_cast<const PacketVersion *>(pkt.body.data());
                if (pv->version != PROTOCOL_VERSION)
                {
                    LOG_ERROR("Client %d: protocol version mismatch %d != %d",
                              clientId, (int)pv->version, PROTOCOL_VERSION);
                    handleClose(clientId);
                    return;
                }
            }
            continue;
        }

        unsigned char pid = pkt.playerId;

        if (pid >= static_cast<unsigned char>(engine.getPlayerCount()))
        {
            LOG_WARN("Player %d: invalid pid=%d", clientId, (int)pid);
            continue;
        }

        if (pkt.length < 2)
        {
            LOG_WARN("Player %d: packet too short", clientId);
            continue;
        }

        if (static_cast<int>(pkt.body.size()) > 4096)
        {
            LOG_WARN("Player %d: packet body too large: %zu", clientId, pkt.body.size());
            continue;
        }

        switch (pkt.type)
        {
            case PacketType::PlayCard:
            {
                if (pkt.body.size() >= sizeof(PacketPlayCard))
                {
                    const PacketPlayCard * pc = reinterpret_cast<const PacketPlayCard *>(pkt.body.data());
                    if (pc->chosenColor >= 1 && pc->chosenColor <= 4)
                    {
                        engine.playCard(pid, pc->cardIndex, static_cast<CardColor>(pc->chosenColor));
                        broadcastSyncState();
                    }
                }
                break;
            }
            case PacketType::Draw:
                if (pkt.body.size() == 0)
                {
                    engine.drawCard(pid);
                    broadcastSyncState();
                }
                break;
            case PacketType::JumpIn:
                if (pkt.body.size() >= sizeof(PacketJumpIn))
                {
                    const PacketJumpIn * pj = reinterpret_cast<const PacketJumpIn *>(pkt.body.data());
                    engine.jumpIn(pid, pj->cardIndex);
                    broadcastSyncState();
                }
                break;
            case PacketType::CallUno:
                if (pkt.body.size() == 0)
                    engine.callUno(pid);
                break;
            case PacketType::CatchUno:
                if (pkt.body.size() >= sizeof(PacketUno))
                {
                    const PacketUno * pu = reinterpret_cast<const PacketUno *>(pkt.body.data());
                    if (pu->targetId < static_cast<unsigned char>(engine.getPlayerCount()))
                    {
                        engine.catchUno(pid, pu->targetId);
                        broadcastSyncState();
                    }
                }
                break;
            default:
                LOG_WARN("Player %d: unknown packet type %d", clientId, static_cast<int>(pkt.type));
                break;
        }
    }

    if (engine.isGameOver())
    {
        broadcastSyncState();
        running = false;
    }
}

void NetworkServer::handleWrite(int clientId)
{
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    Client & ctx = it->second;
    int n = ctx.sendBuf.flush(ctx.fd);
    if (n < 0) handleClose(clientId);

    if (ctx.sendBuf.idle())
    {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        ev.data.u32 = clientId + 1;
        epoll_ctl(epollFd, EPOLL_CTL_MOD, ctx.fd, &ev);
    }
}

void NetworkServer::handleClose(int clientId)
{
    auto it = clients.find(clientId);
    if (it == clients.end()) return;

    Client & ctx = it->second;
    if (ctx.fd >= 0)
    {
        epoll_ctl(epollFd, EPOLL_CTL_DEL, ctx.fd, nullptr);
        close(ctx.fd);
    }
    clients.erase(clientId);
    clientCount--;
    LOG_INFO("Player %d disconnected", clientId);
}

void NetworkServer::broadcastSyncState()
{
    GameState gs = engine.getState();

    int playerDataSize = 0;
    for (int i = 0; i < engine.getPlayerCount(); i++)
    {
        const Player * p = engine.getPlayer(i);
        playerDataSize += (int)sizeof(int) + (int)p->getName().length()
                        + (int)sizeof(int) + (int)sizeof(int) + (int)sizeof(int);
        playerDataSize += p->get_size() * (int)sizeof(Card);
    }

    int bodySize = (int)sizeof(int) + (int)sizeof(GameState) + (int)sizeof(int) + playerDataSize;

    for (auto & kv : clients)
    {
        Client & ctx = kv.second;
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
            const Player * p = engine.getPlayer(i);
            std::string name = p->getName();
            int nameLen = (int)name.length();
            int cardCount = p->get_size();
            int pType = static_cast<int>(p->getType());
            int pDiff = static_cast<int>(p->getDifficulty());

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

            bool isSelf = (i == myPlayerId);
            for (int j = 0; j < cardCount; j++)
            {
                Card c;
                if (isSelf)
                    c = p->peek(j);
                else
                    c = Card{ CardColor::Wild, 0 };
                std::memcpy(buf.data() + offset, &c, sizeof(Card));
                offset += (int)sizeof(Card);
            }
        }

        ctx.sendBuf.beginPacket(static_cast<unsigned char>(PacketType::SyncState), 0, bodySize);
        ctx.sendBuf.writeBody(buf.data(), bodySize);
        ctx.sendBuf.flush(ctx.fd);

        if (!ctx.sendBuf.idle())
        {
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
            ev.data.u32 = kv.first + 1;
            epoll_ctl(epollFd, EPOLL_CTL_MOD, ctx.fd, &ev);
        }
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
        engine.addPlayer(pname, PlayerType::Human, 0);
    }

    engine.start();
    broadcastSyncState();

    struct epoll_event events[16];

    while (running && !engine.isGameOver())
    {
        int n = epoll_wait(epollFd, events, 16, 50);
        if (n < 0) continue;

        for (int i = 0; i < n; i++)
        {
            if (events[i].data.u32 == 0)
                handleAccept();
            else
            {
                int clientId = (int)events[i].data.u32 - 1;
                if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
                    handleClose(clientId);
                else if (events[i].events & EPOLLOUT)
                    handleWrite(clientId);
                else if (events[i].events & EPOLLIN)
                    handleRead(clientId);
            }
        }

        if (engine.isGameOver())
        {
            broadcastSyncState();
            break;
        }
    }

    if (engine.isGameOver())
    {
        int winner = engine.getWinner();
        if (winner >= 0)
            LOG_INFO("%s wins!", engine.getPlayer(winner)->getName().c_str());
    }
}
