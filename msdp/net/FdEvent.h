#ifndef __FD_EVENT_H__
#define __FD_EVENT_H__

#include <sys/epoll.h>
#include <functional>

class FdEvent {
public:
    enum TriggerEvent {
        IN_EVENT = EPOLLIN,
        OUT_EVENT = EPOLLOUT
    };

    FdEvent(int fd);
    FdEvent();
    ~FdEvent();
    std::function<void()> handler(TriggerEvent event_type);
    void listen(TriggerEvent event_type, std::function<void()> callback);
    int getFd() {
        return m_fd;
    }
    epoll_event getEpollEvents() {
        return m_listen_events;
    }
protected:
    int m_fd{ -1 };
    epoll_event m_listen_events;
    std::function<void()> m_read_callback;
    std::function<void()> m_write_callback;
    std::function<void()> m_error_callback;
};

#endif