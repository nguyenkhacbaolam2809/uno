#ifndef _WIN32

#include "net_platform.h"
#include <sys/epoll.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <algorithm>

class EpollLoop : public EventLoop {
public:
    EpollLoop() : running(false) {
        epfd = epoll_create1(0);
        if (epfd < 0)
            std::cerr << "epoll_create1 failed: " << errno << std::endl;
    }

    ~EpollLoop() override {
        stop();
        if (epfd >= 0) close(epfd);
    }

    bool watch(socket_t fd, IOEvent event) override {
        if (epfd < 0) return false;
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.data.fd = fd;
        ev.events = EPOLLET;
        if (event == IOEvent::READ || event == IOEvent::ACCEPT)
            ev.events |= EPOLLIN;
        else if (event == IOEvent::WRITE)
            ev.events |= EPOLLOUT;

        int op = watched.count(fd) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        if (epoll_ctl(epfd, op, fd, &ev) < 0)
        {
            std::cerr << "epoll_ctl failed: " << errno << std::endl;
            return false;
        }
        watched.insert(fd);
        return true;
    }

    bool unwatch(socket_t fd, IOEvent event) override {
        if (epfd < 0) return false;
        struct epoll_event ev;
        std::memset(&ev, 0, sizeof(ev));
        ev.data.fd = fd;
        if (event == IOEvent::READ)
            ev.events = EPOLLOUT | EPOLLET;
        else
            ev.events = EPOLLIN | EPOLLET;

        if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) < 0)
            return false;
        return true;
    }

    bool remove(socket_t fd) override {
        if (epfd < 0) return false;
        watched.erase(fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
        return true;
    }

    int dispatch(IOEventData * events, int maxEvents, int timeoutMs) override {
        if (epfd < 0) return -1;
        struct epoll_event evs[64];
        int n = epoll_wait(epfd, evs, std::min(maxEvents, 64), timeoutMs);
        if (n < 0) return -1;

        int count = 0;
        for (int i = 0; i < n && count < maxEvents; i++)
        {
            events[count].fd = evs[i].data.fd;
            if (evs[i].events & (EPOLLERR | EPOLLHUP))
                events[count].type = IOEvent::CLOSE;
            else if (evs[i].events & EPOLLIN)
                events[count].type = (listening.count(evs[i].data.fd))
                                    ? IOEvent::ACCEPT : IOEvent::READ;
            else if (evs[i].events & EPOLLOUT)
                events[count].type = IOEvent::WRITE;
            else
                continue;
            count++;
        }
        return count;
    }

    void stop() override {
        running = false;
    }

    void markListening(socket_t fd) { listening.insert(fd); }

private:
    int epfd;
    bool running;
    std::set<socket_t> watched;
    std::set<socket_t> listening;
};

std::unique_ptr<EventLoop> createEventLoop()
{
    return std::make_unique<EpollLoop>();
}

bool setNonBlocking(socket_t fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

#endif
