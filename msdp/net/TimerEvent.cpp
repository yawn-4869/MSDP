#include <sys/time.h>
#include "TimerEvent.h"
#include "common/Logger.h"
#include "utils.h"

TimerEvent::TimerEvent(int interval, int is_repeated, std::function<void()> cb)
    :m_interval(interval), m_is_repeated(is_repeated), m_task(cb) {
        resetArriveTime();
}

void TimerEvent::resetArriveTime() {
    int64_t now = getNowMs();
    m_arrive_time = now + m_interval;
    // DEBUGLOG("success create timer event, will excute at [%lld]", m_arrive_time);
}