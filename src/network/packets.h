#ifndef PACKETS_H
#define PACKETS_H

const int DEFAULT_PORT = 8888;
const int BUFFER_SIZE = 4096;
const int MAX_CLIENTS = 4;

enum PacketType : unsigned char
{
    PKT_PLAY_CARD,
    PKT_DRAW,
    PKT_JUMP_IN,
    PKT_CALL_UNO,
    PKT_CATCH_UNO,
    PKT_SYNC_STATE
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
