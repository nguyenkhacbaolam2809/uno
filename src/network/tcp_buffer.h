#ifndef TCP_BUFFER_H
#define TCP_BUFFER_H

#include "packets.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <unistd.h>

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
        return (writePos - readPos) >= 4;
    }

    Packet readPacket()
    {
        Packet pkt;
        pkt.type = PacketType::Heartbeat;
        pkt.playerId = 0;
        pkt.length = 0;

        if ((writePos - readPos) < 4)
            return pkt;

        uint32_t totalLen;
        std::memcpy(&totalLen, data.data() + readPos, 4);
        if (totalLen < 4 || totalLen > 65536 || (writePos - readPos) < totalLen)
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
            {
                pkt.body.assign(bodyStart + 4, bodyStart + 4 + bLen);
                readPos += totalLen;
            }
        }

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
        data.clear();
        writePos = 0;
        inPacket = true;

        uint32_t totalLen = 4 + 4 + bodyLen;
        data.resize(totalLen);

        std::memcpy(data.data(), &totalLen, 4);
        data[4] = static_cast<char>(type);
        data[5] = static_cast<char>(playerId);
        uint16_t bLen = static_cast<uint16_t>(bodyLen);
        std::memcpy(data.data() + 6, &bLen, 2);
        writePos = 4 + 4;
        pktSizePos = 0;
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
        int n = ::write(fd, data.data(), writePos);
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
