#ifndef __IO_THREAD_POOL_H__
#define __IO_THREAD_POOL_H__

#include <vector>
#include "IOThread.h"

class IOThreadPool
{
public:
    IOThreadPool(int size);
    ~IOThreadPool();
    void start();
    void join();
    IOThread* getIOThread() {
        if (m_index == (int)m_io_thread_pool.size() || m_index == -1)  {
            m_index = 0;
        }
        return m_io_thread_pool[m_index++];
    }
private:
    int m_size{ 0 };
    std::vector<IOThread*> m_io_thread_pool;
    int m_index{ -1 };
};


#endif