#include "network_socket.h"
#include "logger.h"
#include <cstring>
#include <cerrno>
#include <algorithm>

// --- SocketInit ---
SocketInit::SocketInit()
{
#ifdef _WIN32
    WSADATA wsaData;
    m_ok = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
    if (!m_ok)
        LOG_ERROR("WSAStartup failed: %d", WSAGetLastError());
    else
        LOG_INFO("Winsock initialized");
#else
    m_ok = true;
#endif
}

SocketInit::~SocketInit()
{
#ifdef _WIN32
    if (m_ok)
        WSACleanup();
#endif
}

// --- TcpSocket ---
TcpSocket::TcpSocket() : m_fd(INVALID_SOCK) {}

TcpSocket::TcpSocket(socket_t fd) : m_fd(fd) {}

TcpSocket::~TcpSocket() { close(); }

TcpSocket::TcpSocket(TcpSocket && other) noexcept
    : m_fd(other.m_fd)
{
    other.m_fd = INVALID_SOCK;
}

TcpSocket & TcpSocket::operator=(TcpSocket && other) noexcept
{
    if (this != &other)
    {
        close();
        m_fd = other.m_fd;
        other.m_fd = INVALID_SOCK;
    }
    return *this;
}

bool TcpSocket::setNonBlocking(bool enabled)
{
#ifdef _WIN32
    u_long mode = enabled ? 1 : 0;
    return ioctlsocket(m_fd, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(m_fd, F_GETFL, 0);
    if (flags < 0) return false;
    flags = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(m_fd, F_SETFL, flags) >= 0;
#endif
}

bool TcpSocket::setReuseAddr()
{
    int opt = 1;
#ifdef _WIN32
    return setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == 0;
#else
    return setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
#endif
}

bool TcpSocket::connect(const std::string & host, int port, int timeoutMs)
{
    if (m_fd != INVALID_SOCK) close();

    struct addrinfo hints, * res = nullptr;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::string portStr = std::to_string(port);
    int err = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (err != 0 || !res)
    {
        LOG_ERROR("getaddrinfo failed for %s: %s", host.c_str(), gai_strerror(err));
        return false;
    }

    m_fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (m_fd == INVALID_SOCK)
    {
        LOG_ERROR("socket() failed: errno=%d", errno);
        freeaddrinfo(res);
        return false;
    }

    setNonBlocking(true);

    int rc = ::connect(m_fd, res->ai_addr, (int)res->ai_addrlen);
    freeaddrinfo(res);

#ifdef _WIN32
    if (rc != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#else
    if (rc != 0 && errno != EINPROGRESS)
#endif
    {
        close();
        return false;
    }

    if (rc == 0)
    {
        setNonBlocking(false);
        return true;
    }

    SocketPoller poller;
    poller.add(m_fd, false, true);
    int ready = poller.wait(timeoutMs);
    if (ready <= 0)
    {
        LOG_WARN("connect to %s:%d timed out", host.c_str(), port);
        close();
        return false;
    }

    int soErr = 0;
    socklen_t errLen = sizeof(soErr);
    getsockopt(m_fd, SOL_SOCKET, SO_ERROR, (char *)&soErr, &errLen);
    if (soErr != 0)
    {
        LOG_ERROR("connect to %s:%d failed: errno=%d", host.c_str(), port, soErr);
        close();
        return false;
    }

    setNonBlocking(false);
    return true;
}

void TcpSocket::close()
{
    if (m_fd == INVALID_SOCK) return;
#ifdef _WIN32
    ::closesocket(m_fd);
#else
    ::close(m_fd);
#endif
    m_fd = INVALID_SOCK;
}

int TcpSocket::send(const char * data, int len)
{
#ifdef _WIN32
    return ::send(m_fd, data, len, 0);
#else
    return (int)::write(m_fd, data, len);
#endif
}

int TcpSocket::recv(char * buf, int len)
{
#ifdef _WIN32
    return ::recv(m_fd, buf, len, 0);
#else
    return (int)::read(m_fd, buf, len);
#endif
}

// --- TcpListener ---
TcpListener::TcpListener() : m_fd(INVALID_SOCK) {}

TcpListener::~TcpListener() { close(); }

TcpListener::TcpListener(TcpListener && other) noexcept
    : m_fd(other.m_fd)
{
    other.m_fd = INVALID_SOCK;
}

TcpListener & TcpListener::operator=(TcpListener && other) noexcept
{
    if (this != &other)
    {
        close();
        m_fd = other.m_fd;
        other.m_fd = INVALID_SOCK;
    }
    return *this;
}

bool TcpListener::listen(int port, int backlog)
{
    if (m_fd != INVALID_SOCK) close();

    m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd == INVALID_SOCK) return false;

    setReuseAddr();

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(m_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        LOG_ERROR("bind failed on port %d", port);
        close();
        return false;
    }

    if (::listen(m_fd, backlog) < 0)
    {
        close();
        return false;
    }

    return true;
}

TcpSocket TcpListener::accept()
{
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    socket_t clientFd = ::accept(m_fd, (struct sockaddr *)&addr, &addrLen);
    if (clientFd == INVALID_SOCK) return TcpSocket();
    return TcpSocket(clientFd);
}

void TcpListener::close()
{
    if (m_fd == INVALID_SOCK) return;
#ifdef _WIN32
    ::closesocket(m_fd);
#else
    ::close(m_fd);
#endif
    m_fd = INVALID_SOCK;
}

bool TcpListener::setReuseAddr()
{
    int opt = 1;
#ifdef _WIN32
    return setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt)) == 0;
#else
    return setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
#endif
}

// --- SocketPoller ---
SocketPoller::SocketPoller()
{
#ifdef _WIN32
    m_count = 0;
#else
    m_epollFd = epoll_create1(0);
#endif
}

SocketPoller::~SocketPoller()
{
#ifndef _WIN32
    if (m_epollFd >= 0) ::close(m_epollFd);
#endif
}

bool SocketPoller::add(socket_t fd, bool wantRead, bool wantWrite)
{
#ifdef _WIN32
    FD_SET(fd, &m_readSet);
    if (wantWrite) FD_SET(fd, &m_writeSet);
    FD_SET(fd, &m_errSet);
    m_maxFd = (fd > m_maxFd) ? fd : m_maxFd;
    m_count++;
    return true;
#else
    struct epoll_event ev;
    ev.events = 0;
    if (wantRead)  ev.events |= EPOLLIN;
    if (wantWrite) ev.events |= EPOLLOUT;
    ev.data.u64 = 0;
    ev.data.fd = fd;
    int rc = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev);
    return rc == 0;
#endif
}

bool SocketPoller::modify(socket_t fd, bool wantRead, bool wantWrite)
{
#ifdef _WIN32
    (void)fd; (void)wantRead; (void)wantWrite;
    return true;
#else
    struct epoll_event ev;
    ev.events = 0;
    if (wantRead)  ev.events |= EPOLLIN;
    if (wantWrite) ev.events |= EPOLLOUT;
    ev.data.fd = fd;
    return epoll_ctl(m_epollFd, EPOLL_CTL_MOD, fd, &ev) == 0;
#endif
}

bool SocketPoller::remove(socket_t fd)
{
#ifdef _WIN32
    FD_CLR(fd, &m_readSet);
    FD_CLR(fd, &m_writeSet);
    FD_CLR(fd, &m_errSet);
    m_count--;
    return true;
#else
    return epoll_ctl(m_epollFd, EPOLL_CTL_DEL, fd, nullptr) == 0;
#endif
}

int SocketPoller::wait(int timeoutMs)
{
#ifdef _WIN32
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    fd_set readCopy = m_readSet;
    fd_set writeCopy = m_writeSet;
    fd_set errCopy = m_errSet;

    int n = select((int)m_maxFd + 1, &readCopy, &writeCopy, &errCopy, &tv);
    if (n <= 0) return n;

    m_resultCount = 0;
    for (socket_t fd = 0; fd <= m_maxFd && m_resultCount < MAX_EVENTS; fd++)
    {
        PollEvent ev = PollEvent::None;
        if (FD_ISSET(fd, &errCopy))
            ev = PollEvent::Error;
        else if (FD_ISSET(fd, &readCopy))
            ev = PollEvent::Readable;
        else if (FD_ISSET(fd, &writeCopy))
            ev = PollEvent::Writable;

        if (ev != PollEvent::None)
        {
            m_results[m_resultCount].fd = fd;
            m_results[m_resultCount].event = ev;
            m_resultCount++;
        }
    }
    return m_resultCount;
#else
    int n = epoll_wait(m_epollFd, m_events, MAX_EVENTS, timeoutMs);
    if (n < 0 && errno == EINTR) return 0;
    m_count = (n > 0) ? n : 0;
    return m_count;
#endif
}

PollEvent SocketPoller::getEvent(int index) const
{
#ifdef _WIN32
    if (index < 0 || index >= m_resultCount) return PollEvent::None;
    return m_results[index].event;
#else
    if (index < 0 || index >= m_count) return PollEvent::None;
    uint32_t ev = m_events[index].events;
    if (ev & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
        return (ev & EPOLLIN) ? PollEvent::Readable : PollEvent::Disconnected;
    if (ev & EPOLLIN)  return PollEvent::Readable;
    if (ev & EPOLLOUT) return PollEvent::Writable;
    return PollEvent::None;
#endif
}

socket_t SocketPoller::getFd(int index) const
{
#ifdef _WIN32
    if (index < 0 || index >= m_resultCount) return INVALID_SOCK;
    return m_results[index].fd;
#else
    if (index < 0 || index >= m_count) return INVALID_SOCK;
    return m_events[index].data.fd;
#endif
}
