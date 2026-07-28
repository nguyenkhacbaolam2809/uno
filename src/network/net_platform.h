#ifndef NET_PLATFORM_H
#define NET_PLATFORM_H

#include "packets.h"
#include <memory>
#include <functional>
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
    constexpr int SOCK_ERR = SOCKET_ERROR;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <cerrno>
    using socket_t = int;
    constexpr socket_t INVALID_SOCK = -1;
    constexpr int SOCK_ERR = -1;
    inline void closesocket(socket_t fd) { close(fd); }
    inline int WSAGetLastError() { return errno; }
#endif

enum class IOEvent { READ, WRITE, ACCEPT, ERR, CLOSE };

struct IOEventData {
    socket_t fd;
    IOEvent type;
};

class EventLoop {
public:
    virtual ~EventLoop() = default;
    virtual bool watch(socket_t fd, IOEvent event) = 0;
    virtual bool unwatch(socket_t fd, IOEvent event) = 0;
    virtual bool remove(socket_t fd) = 0;
    virtual int dispatch(IOEventData * events, int maxEvents, int timeoutMs) = 0;
    virtual void stop() = 0;
};

std::unique_ptr<EventLoop> createEventLoop();

bool setNonBlocking(socket_t fd);

struct TcpReader {
    std::vector<char> readBuf;

    TcpReader();
    bool readPacket(socket_t fd);

    PacketType packetType() const;
    unsigned char playerId() const;
    const char * body() const;
    int bodyLen() const;

private:
    enum State { HEADER, BODY, DONE };
    State state;
    int headerReceived;
    int bodyReceived;
    PacketHeader hdr;
    std::vector<char> bodyData;
};

struct TcpWriter {
    std::vector<char> writeBuf;
    int written;

    TcpWriter();
    void beginPacket(PacketType type, unsigned char playerId, int bodySize);
    void writeBody(const void * data, int len);
    int sendPending(socket_t fd);
    bool isIdle() const;
};

#endif
