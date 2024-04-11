#include <sys/syscall.h>
#include <unistd.h>
#include <assert.h>
#include "IOThread.h"
#include "Logger.h"

IOThread::IOThread() {
    int rt = sem_init(&m_init_sem, 0, 0);
    assert(rt == 0);

    rt = sem_init(&m_start_sem, 0, 0);
    assert(rt == 0);

    pthread_create(&m_thread, NULL, Run, this);

    // wait. 直到新线程执行完Run函数的前置: eventloop创建成功
    sem_wait(&m_init_sem);
    DEBUGLOG("IOThread [%d] create success", m_thread_id);
}

IOThread::~IOThread() {
    m_event_loop->stop();

    sem_destroy(&m_init_sem);
    sem_destroy(&m_start_sem);

    pthread_join(m_thread, NULL);

    if(m_event_loop){
        delete(m_event_loop);
        m_event_loop = NULL;
    }
}

void* IOThread::Run(void* arg) {
    IOThread* thread = static_cast<IOThread*>(arg);

    thread->m_event_loop = new EventLoop();
    thread->m_thread_id = syscall(SYS_gettid);

    // 唤醒等待线程
    sem_post(&thread->m_init_sem);
    DEBUGLOG("IOThread [%d] created wait to start", thread->m_thread_id);

    // 让IO线程等待，直到主动发起启动, 而不是创建好线程就直接启动
    sem_wait(&thread->m_start_sem);
    DEBUGLOG("IOThread [%d] start loop", thread->m_thread_id);

    thread->m_event_loop->loop();

    DEBUGLOG("IOThread [%d] end loop ", thread->m_thread_id);
    return NULL;
}

void IOThread::start() {
    DEBUGLOG("Now invoke IOThread [%d]", m_thread_id);
    sem_post(&m_start_sem);
}

void IOThread::join() {
  pthread_join(m_thread, NULL);
}