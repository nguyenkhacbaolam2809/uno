#ifndef NETWORK_SOCKET_H
#define NETWORK_SOCKET_H

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef SOCKET socket_t;
    static constexpr socket_t INVALID_SOCK = INVALID_SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <fcntl.h>
    typedef int socket_t;
    static constexpr socket_t INVALID_SOCK = -1;
#endif

class TcpSocket {
public:
    TcpSocket();
    explicit TcpSocket(socket_t fd);
    ~TcpSocket();

    TcpSocket(TcpSocket && other) noexcept;
    TcpSocket & operator=(TcpSocket && other) noexcept;
    TcpSocket(const TcpSocket &) = delete;
    TcpSocket & operator=(const TcpSocket &) = delete;

    bool connect(const std::string & host, int port, int timeoutMs = 3000);
    void close();
    bool isOpen() const noexcept { return m_fd != INVALID_SOCK; }
    socket_t fd() const noexcept { return m_fd; }

    int send(const char * data, int len);
    int recv(char * buf, int len);

    bool setNonBlocking(bool enabled);
    bool setReuseAddr();

private:
    socket_t m_fd;
};

class TcpListener {
public:
    TcpListener();
    ~TcpListener();

    TcpListener(TcpListener && other) noexcept;
    TcpListener & operator=(TcpListener && other) noexcept;
    TcpListener(const TcpListener &) = delete;
    TcpListener & operator=(const TcpListener &) = delete;

    bool listen(int port, int backlog = 8);
    TcpSocket accept();
    void close();
    bool isOpen() const noexcept { return m_fd != INVALID_SOCK; }
    socket_t fd() const noexcept { return m_fd; }

private:
    socket_t m_fd;
};

enum class PollEvent { None, Readable, Writable, Error, Disconnected };

class SocketPoller {
public:
    SocketPoller();
    ~SocketPoller();

    bool add(socket_t fd, bool wantRead, bool wantWrite);
    bool modify(socket_t fd, bool wantRead, bool wantWrite);
    bool remove(socket_t fd);
    int wait(int timeoutMs);
    PollEvent getEvent(int index) const;
    socket_t getFd(int index) const;
    int count() const noexcept { return m_count; }

private:
    static constexpr int MAX_EVENTS = 64;

#ifdef _WIN32
    fd_set m_readSet;
    fd_set m_writeSet;
    fd_set m_errSet;
    socket_t m_maxFd{INVALID_SOCK};
    int m_count{0};
    struct WinPollResult {
        socket_t fd;
        PollEvent event;
    } m_results[MAX_EVENTS];
    int m_resultCount{0};
#else
    int m_epollFd{-1};
    struct epoll_event m_events[MAX_EVENTS];
    int m_count{0};
#endif
};

class SocketInit {
public:
    SocketInit();
    ~SocketInit();
    bool ok() const noexcept { return m_ok; }
private:
    bool m_ok{false};
};

#endif
