#ifdef _WIN32

#include "net_platform.h"
#include <iostream>
#include <set>

class SelectLoop : public EventLoop {
public:
    SelectLoop() : running(false) {}
    ~SelectLoop() override = default;

    bool watch(socket_t fd, IOEvent event) override {
        if (event == IOEvent::READ || event == IOEvent::ACCEPT)
            readSet.insert(fd);
        else if (event == IOEvent::WRITE)
            writeSet.insert(fd);
        return true;
    }

    bool unwatch(socket_t fd, IOEvent event) override {
        if (event == IOEvent::READ || event == IOEvent::ACCEPT)
            readSet.erase(fd);
        else if (event == IOEvent::WRITE)
            writeSet.erase(fd);
        return true;
    }

    bool remove(socket_t fd) override {
        readSet.erase(fd);
        writeSet.erase(fd);
        return true;
    }

    int dispatch(IOEventData * events, int maxEvents, int timeoutMs) override {
        fd_set readFDs, writeFDs, errFDs;
        FD_ZERO(&readFDs);
        FD_ZERO(&writeFDs);
        FD_ZERO(&errFDs);

        socket_t maxFd = 0;
        for (socket_t fd : readSet) { FD_SET(fd, &readFDs); FD_SET(fd, &errFDs); if (fd > maxFd) maxFd = fd; }
        for (socket_t fd : writeSet) { FD_SET(fd, &writeFDs); if (fd > maxFd) maxFd = fd; }

        struct timeval tv;
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;

        int n = select(static_cast<int>(maxFd + 1), &readFDs, &writeFDs, &errFDs,
                       timeoutMs >= 0 ? &tv : nullptr);
        if (n <= 0) return n;

        int count = 0;
        for (socket_t fd : readSet)
        {
            if (count >= maxEvents) break;
            if (FD_ISSET(fd, &errFDs))
            {
                events[count].fd = fd;
                events[count].type = IOEvent::CLOSE;
                count++;
            }
            else if (FD_ISSET(fd, &readFDs))
            {
                events[count].fd = fd;
                events[count].type = (listening.count(fd)) ? IOEvent::ACCEPT : IOEvent::READ;
                count++;
            }
        }
        for (socket_t fd : writeSet)
        {
            if (count >= maxEvents) break;
            if (FD_ISSET(fd, &writeFDs))
            {
                events[count].fd = fd;
                events[count].type = IOEvent::WRITE;
                count++;
            }
        }
        return count;
    }

    void stop() override { running = false; }
    void markListening(socket_t fd) { listening.insert(fd); }

private:
    std::set<socket_t> readSet;
    std::set<socket_t> writeSet;
    std::set<socket_t> listening;
    bool running;
};

std::unique_ptr<EventLoop> createEventLoop()
{
    return std::make_unique<SelectLoop>();
}

bool setNonBlocking(socket_t fd)
{
    u_long mode = 1;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
}

#endif
