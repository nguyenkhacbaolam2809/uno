#include "net_platform.h"
#include <cstring>

TcpReader::TcpReader()
    : state(HEADER), headerReceived(0), bodyReceived(0)
{
    std::memset(&hdr, 0, sizeof(hdr));
}

bool TcpReader::readPacket(socket_t fd)
{
    while (true)
    {
        if (state == HEADER)
        {
            int need = static_cast<int>(sizeof(PacketHeader)) - headerReceived;
            int got = recv(fd, reinterpret_cast<char*>(&hdr) + headerReceived, need, 0);
            if (got <= 0) return false;
            headerReceived += got;
            if (headerReceived >= static_cast<int>(sizeof(PacketHeader)))
            {
                if (hdr.bodyLen > BUFFER_SIZE)
                {
                    state = DONE;
                    return false;
                }
                bodyData.resize(hdr.bodyLen);
                state = BODY;
            }
        }

        if (state == BODY)
        {
            int need = hdr.bodyLen - bodyReceived;
            if (need == 0)
            {
                state = DONE;
                break;
            }
            int got = recv(fd, bodyData.data() + bodyReceived, need, 0);
            if (got <= 0) return false;
            bodyReceived += got;
            if (bodyReceived >= hdr.bodyLen)
            {
                state = DONE;
                break;
            }
            if (got < need)
                return true;
        }

        if (state == DONE)
            break;
    }
    return state == DONE;
}

PacketType TcpReader::packetType() const { return hdr.type; }
unsigned char TcpReader::playerId() const { return hdr.playerId; }
const char * TcpReader::body() const { return bodyData.data(); }
int TcpReader::bodyLen() const { return hdr.bodyLen; }


TcpWriter::TcpWriter() : written(0) {}

void TcpWriter::beginPacket(PacketType type, unsigned char playerId, int bodySize)
{
    writeBuf.clear();
    written = 0;

    PacketHeader hdr;
    hdr.type = type;
    hdr.playerId = playerId;
    hdr.bodyLen = static_cast<unsigned short>(bodySize);

    writeBuf.resize(sizeof(PacketHeader) + bodySize);
    std::memcpy(writeBuf.data(), &hdr, sizeof(PacketHeader));
}

void TcpWriter::writeBody(const void * data, int len)
{
    std::memcpy(writeBuf.data() + sizeof(PacketHeader), data, len);
}

int TcpWriter::sendPending(socket_t fd)
{
    if (writeBuf.empty()) return 0;
    int remaining = static_cast<int>(writeBuf.size()) - written;
    if (remaining <= 0) { writeBuf.clear(); written = 0; return 0; }

    int sent = send(fd, writeBuf.data() + written, remaining, 0);
    if (sent <= 0) return SOCK_ERR;
    written += sent;

    if (written >= static_cast<int>(writeBuf.size()))
    {
        writeBuf.clear();
        written = 0;
    }
    return written;
}

bool TcpWriter::isIdle() const { return writeBuf.empty(); }
