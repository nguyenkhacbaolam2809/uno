#ifndef PACKETS_H
#define PACKETS_H

#include <cstdint>

constexpr int DEFAULT_PORT = 8888;
constexpr int BUFFER_SIZE = 4096;
constexpr int MAX_CLIENTS = 4;
constexpr int PROTOCOL_VERSION = 1;

enum class PacketType : unsigned char
{
    Heartbeat,
    PlayCard,
    Draw,
    JumpIn,
    CallUno,
    CatchUno,
    SyncState
};

struct PacketVersion
{
    unsigned char version;
};

#pragma pack(push, 1)
struct PacketHeader
{
    PacketType type;
    unsigned char playerId;
    unsigned short bodyLen;
};

struct PacketPlayCard
{
    unsigned char cardIndex;
    unsigned char chosenColor;
};

struct PacketJumpIn
{
    unsigned char cardIndex;
};

struct PacketUno
{
    unsigned char targetId;
};
#pragma pack(pop)

#endif
