#include <sys/timerfd.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "Timer.h"
#include "Logger.h"
#include "utils.h"

Timer::Timer() : FdEvent() {
    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    // DEBUGLOG("timer fd=%d", m_fd);
    listen(FdEvent::IN_EVENT, std::bind(&Timer::onTimer, this));
}

Timer::~Timer() {

}

void Timer::addTimerEvent(TimerEvent::s_ptr event) {
    bool is_reset_timerfd = false;

    std::unique_lock<std::mutex> lck(m_mutex);
    if(m_pending_events.empty()) {
        is_reset_timerfd = true;
    } else {
        auto it = m_pending_events.begin();
        // 如果当前插入事件的到期时间小于队首事件的到期时间，需要重新设置监听事件
        if((*it).second->getArriveTime() > event->getArriveTime()) {
            is_reset_timerfd = true;
        }
    }
    
    m_pending_events.emplace(event->getArriveTime(), event);
    lck.unlock();
    if(is_reset_timerfd) {
        resetArriveTime();
    }
}

void Timer::deleteTimerEvent(TimerEvent::s_ptr event){
    event->setCancel(true);
    std::unique_lock<std::mutex> lck(m_mutex);
    auto begin = m_pending_events.lower_bound(event->getArriveTime());
    auto end = m_pending_events.upper_bound(event->getArriveTime());

    auto it = begin;
    for(it = begin; it != end; it++) {
        if(it->second = event) {
            break;
        }
    }
    if(it != end) {
        m_pending_events.erase(it);
    }
    lck.unlock();

}

void Timer::onTimer() {
    char buf[8];
    // 处理缓冲区数据, 防止下一次继续触发刻度时间
    while(1) {
        if(read(m_fd, buf, 8) == -1 && errno == EAGAIN) {
            break;
        }
    }

    int64_t now = getNowMs();
    std::vector<TimerEvent::s_ptr> tmp_events;
    std::vector<std::pair<int64_t, std::function<void()>>> tmp_tasks;
    std::unique_lock<std::mutex> lck(m_mutex);
    auto it = m_pending_events.begin();
    while(it != m_pending_events.end()) {
        if(it->first > now) {
            break;
        }
        if(!it->second->isCancel()) {
            tmp_events.push_back(it->second);
            tmp_tasks.push_back(std::make_pair(it->first, it->second->getCallback()));
        }
        it++;
    }
    m_pending_events.erase(m_pending_events.begin(), it);
    lck.unlock();

    // 需要把需要重复执行的任务再添加进去
    for(auto it = tmp_events.begin(); it != tmp_events.end(); it++) {
        if((*it)->isRepeated()) {
            (*it)->resetArriveTime();
            addTimerEvent(*it);
        }
    }

    // 重新设置调用时间
    resetArriveTime();

    // 执行任务
    for(auto it = tmp_tasks.begin(); it != tmp_tasks.end(); it++) {
        it->second();
    }
}

void Timer::resetArriveTime() {
    std::unique_lock<std::mutex> lck(m_mutex);
    auto tmp = m_pending_events;
    lck.unlock();

    if(tmp.empty()) {
        return;
    }

    auto it = tmp.begin();
    
    int64_t now = getNowMs();
    int64_t interval = 0;
    if(it->second->getArriveTime() > now) {
        interval = it->second->getArriveTime() - now;
    } else {
        interval = 100;
    }
    timespec ts;
    memset(&ts, 0, sizeof(ts));
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;
    // itimerspec: it_value: 第一次调用的时间 it_interval: 第一次调用后重复调用的间隔时间
    itimerspec value;
    memset(&value, 0, sizeof(value));
    value.it_value = ts;
    int rt = timerfd_settime(m_fd, 0, &value, NULL);
    if(rt != 0) {
        ERRORLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
    }
    // DEBUGLOG("timer reset to %lld", now + interval);
}