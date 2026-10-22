#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include "packets.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>

struct Packet {
    PacketType type;
    unsigned char playerId;
    int length;
    std::vector<char> body;
};

class RecvBuffer
{
public:
    static constexpr size_t INITIAL_SIZE = 16384;

    RecvBuffer() : data(INITIAL_SIZE), readPos(0), writePos(0) {}

    int fill(int fd)
    {
        if (writePos >= data.size())
        {
            if (readPos > 0)
            {
                std::memmove(data.data(), data.data() + readPos, writePos - readPos);
                writePos -= readPos;
                readPos = 0;
            }
            else
                data.resize(data.size() * 2);
        }

        int n = ::read(fd, data.data() + writePos, data.size() - writePos);
        if (n > 0)
            writePos += n;
        return n;
    }

    bool hasPacket() const
    {
        const size_t available = writePos - readPos;
        if (available < sizeof(uint32_t))
            return false;

        uint32_t totalLen;
        std::memcpy(&totalLen, data.data() + readPos, sizeof(totalLen));
        if (totalLen < 8 || totalLen > 65536)
            return true;
        return available >= totalLen;
    }

    Packet readPacket()
    {
        Packet pkt;
        pkt.type = PacketType::Heartbeat;
        pkt.playerId = 0;
        pkt.length = 0;

        if ((writePos - readPos) < sizeof(uint32_t))
            return pkt;

        uint32_t totalLen;
        std::memcpy(&totalLen, data.data() + readPos, 4);
        if (totalLen < 8 || totalLen > 65536)
        {
            readPos += sizeof(uint32_t);
            return pkt;
        }
        if ((writePos - readPos) < totalLen)
            return pkt;

        int bodyOffset = readPos + 4;
        int bodyMaxLen = totalLen - 4;

        if (bodyMaxLen >= 4)
        {
            const char * bodyStart = data.data() + bodyOffset;
            pkt.type = static_cast<PacketType>(static_cast<unsigned char>(bodyStart[0]));
            pkt.playerId = static_cast<unsigned char>(bodyStart[1]);
            uint16_t bLen;
            std::memcpy(&bLen, bodyStart + 2, 2);
            pkt.length = 4 + bLen;

            if (bodyMaxLen >= 4 + bLen)
                pkt.body.assign(bodyStart + 4, bodyStart + 4 + bLen);
        }

        readPos += totalLen;
        return pkt;
    }

private:
    std::vector<char> data;
    size_t readPos;
    size_t writePos;
};

class SendBuffer
{
public:
    static constexpr size_t INITIAL_SIZE = 16384;

    SendBuffer() : data(INITIAL_SIZE), writePos(0), pktSizePos(0), inPacket(false) {}

    void beginPacket(unsigned char type, unsigned char playerId, int bodyLen)
    {
        // Append instead of clearing: a previous sync may still be partially
        // unsent when the next state is broadcast.
        size_t start = writePos;
        uint32_t totalLen = 4 + 4 + bodyLen;
        data.resize(start + totalLen);

        std::memcpy(data.data() + start, &totalLen, 4);
        data[start + 4] = static_cast<char>(type);
        data[start + 5] = static_cast<char>(playerId);
        uint16_t bLen = static_cast<uint16_t>(bodyLen);
        std::memcpy(data.data() + start + 6, &bLen, 2);
        writePos = start + 8;
        pktSizePos = start;
        inPacket = true;
    }

    void writeBody(const void * buf, int len)
    {
        if (inPacket && buf && len > 0)
        {
            std::memcpy(data.data() + writePos, buf, len);
            writePos += len;
        }
    }

    int flush(int fd)
    {
        if (writePos == 0) return 0;
        // Never let a disconnected peer raise SIGPIPE and kill the process.
        int n = (int)::send(fd, data.data(), writePos, MSG_NOSIGNAL);
        if (n > 0)
        {
            if (n < static_cast<int>(writePos))
            {
                std::memmove(data.data(), data.data() + n, writePos - n);
                writePos -= n;
            }
            else
                writePos = 0;
        }
        return n;
    }

    bool idle() const { return writePos == 0; }

private:
    std::vector<char> data;
    size_t writePos;
    size_t pktSizePos;
    bool inPacket;
};

#endif
