#ifndef __LOCKER_H__
#define __LOCKER_H__

#include <pthread.h>
#include <exception>

class Locker {
public:
    Locker() {
        if(pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~Locker() {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t* getMutex() {
        return &m_mutex;
    }

private: 
    pthread_mutex_t m_mutex;
};

#endif