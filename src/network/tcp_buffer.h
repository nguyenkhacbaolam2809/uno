#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

constexpr int BUFFER_SIZE = 65536;

struct RecvBuffer {
    std::vector<char> buf;
    int pending;

    RecvBuffer() : buf(BUFFER_SIZE), pending(0) {}

    int fill(int fd)
    {
        if (pending >= BUFFER_SIZE) return -1;
        int n = recv(fd, buf.data() + pending, BUFFER_SIZE - pending, 0);
        if (n > 0) pending += n;
        return n;
    }

    bool hasPacket() const
    {
        if (pending < 6) return false; // len(4) + type(1) + pid(1)
        uint32_t pktLen;
        std::memcpy(&pktLen, buf.data(), sizeof(pktLen));
        pktLen = ntohl(pktLen);
        return pending >= static_cast<int>(pktLen + 4);
    }

    struct Packet {
        uint32_t length;
        unsigned char type;
        unsigned char playerId;
        std::vector<char> body;
    };

    Packet readPacket()
    {
        Packet p;
        uint32_t netLen;
        std::memcpy(&netLen, buf.data(), sizeof(netLen));
        p.length = ntohl(netLen);
        p.type = static_cast<unsigned char>(buf[4]);
        p.playerId = static_cast<unsigned char>(buf[5]);
        int bodyLen = p.length - 2;
        if (bodyLen > 0)
        {
            p.body.assign(buf.data() + 6, buf.data() + 6 + bodyLen);
        }
        int total = p.length + 4;
        pending -= total;
        if (pending > 0)
            std::memmove(buf.data(), buf.data() + total, pending);
        return p;
    }

    void consume(int bytes)
    {
        pending -= bytes;
        if (pending > 0)
            std::memmove(buf.data(), buf.data() + bytes, pending);
    }
};

struct SendBuffer {
    std::vector<char> buf;
    int sent;

    SendBuffer() : sent(0) {}

    void clear() { buf.clear(); sent = 0; }

    void beginPacket(unsigned char type, unsigned char playerId, int bodyLen)
    {
        clear();
        uint32_t totalLen = bodyLen + 2;
        uint32_t netLen = htonl(totalLen);
        buf.resize(totalLen + 4);
        std::memcpy(buf.data(), &netLen, 4);
        buf[4] = static_cast<char>(type);
        buf[5] = static_cast<char>(playerId);
    }

    void writeBody(const void * data, int len)
    {
        std::memcpy(buf.data() + 6, data, len);
    }

    int flush(int fd)
    {
        if (buf.empty()) return 0;
        int remaining = static_cast<int>(buf.size()) - sent;
        if (remaining <= 0) { clear(); return 0; }
        int n = send(fd, buf.data() + sent, remaining, 0);
        if (n > 0) sent += n;
        if (sent >= static_cast<int>(buf.size())) clear();
        return n;
    }

    bool idle() const { return buf.empty(); }
};

#endif
