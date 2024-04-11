#ifndef __Timer_H__
#define __Timer_H__

#include <map>
#include <mutex>
#include "FdEvent.h"
#include "TimerEvent.h"

class Timer : public FdEvent {
public:
    Timer();
    ~Timer();

    void addTimerEvent(TimerEvent::s_ptr event);
    void deleteTimerEvent(TimerEvent::s_ptr event);
    void onTimer(); // 发生IO事件后, eventloop执行的回调函数

private:
    void resetArriveTime();
private:
    std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events;
    std::mutex m_mutex;
};

#endif