#ifndef __EVENT_LOOP_H__
#define __EVENT_LOOP_H__

#include <pthread.h>
#include <set>
#include <queue>
#include <functional>
#include "FdEvent.h"
#include "Timer.h"

class EventLoop
{
public:
    EventLoop();
    ~EventLoop();

public:
    void loop();
    void stop();
    void addEpollEvent(FdEvent* event);
    void addTimerEvent(TimerEvent::s_ptr event);
    void deleteEpollEvent(FdEvent* event);

public: 
    static EventLoop* GetCurrentEventloop();

private:
    bool isInLoopThread();
    void addTask(std::function<void()> callback, bool is_wake_up = false);
    void initTimer();

private:
    pid_t m_thread_id{ 0 }; // 线程id
    int m_epoll_fd{ 0 }; // epoll fd
    bool m_stop_flag{ false }; // 循环停止标志
    std::set<int> m_listen_fds; // 监听事件fd集合
    std::queue<std::function<void()>> m_pending_tasks; // 任务队列
    std::mutex m_mutex; // 互斥锁
    Timer* m_timer;
};

#endif