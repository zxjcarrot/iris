#ifndef IRIS_LOG_H_
#define IRIS_LOG_H_

#include <cmath>
#include <cstdio>

#include <thread>
#include <vector>
#include <memory>
#include <atomic>

#include "define.h"
#include "writer.h"
#include "sslfqueue.h"

namespace iris {

typedef std::shared_ptr<std::vector<char>> sp_loglet_t;
struct thread_logqueue;
extern thread_local std::unique_ptr<thread_logqueue> this_thread_logqueue;
extern thread_local level log_level;


struct thread_logqueue {
    thread_logqueue();                      // used only by normal threads
    thread_logqueue(thread_logqueue *head); // used only by logging thread
    ~thread_logqueue();
    sslfqueue<sp_loglet_t, 5000> q;
    std::atomic<thread_logqueue *> next;
    thread_logqueue * head;
    bool logging_thread;                 // if current thread is the logging thread
};



class logger {
public:
    logger(level l = INFO, writer * pwriter = nullptr);

    void set_level(level l) {
        log_level = l;
    }

    template<typename... Args>
    void log(level l, const char * fmt, Args... args) {
        if (l < log_level)
            return;

        if (iris_unlikely(!this_thread_logqueue.get())) {
            this_thread_logqueue = std::unique_ptr<thread_logqueue>(new thread_logqueue(&m_head));
        }

        sp_loglet_t sl(new std::vector<char>(128));
        std::vector<char> & msg = *sl;
        int final_n, n = 128;
        va_list ap; 

        while (true) {
            final_n = snprintf(&msg[0], msg.size(), fmt, args...);

            if (final_n < 0 || final_n >= n)
                n += std::abs(final_n - n + 1.0); 
            else
                break;
            msg.resize(n);
        }

        msg.resize(final_n);
        msg.push_back('\n');s

        while(!this_thread_logqueue->q.offer(sl))
            std::this_thread::yield();
    }

    template<typename... Args>
    void trace(const char * fmt, Args... args) {
        log(TRACE, fmt, args...);
    }

    template<typename... Args>
    void debug(const char * fmt, Args... args) {
        log(DEBUG, fmt, args...);
    }

    template<typename... Args>
    void info(const char * fmt, Args... args) {
        log(INFO, fmt, args...);
    }

    template<typename... Args>
    void warn(const char * fmt, Args... args) {
        log(WARN, fmt, args...);
    }

    template<typename... Args>
    void error(const char * fmt, Args... args) {
        log(ERROR, fmt, args...);
    }

    template<typename... Args>
    void fatal(const char * fmt, Args... args) {
        log(FATAL, fmt, args...);
    }

    void sync_and_close() {
        if (!m_stop.load(std::memory_order_acquire)) {
            m_stop.store(true, std::memory_order_release);
            m_logging_thread.join();
        }
    }

    ~logger() {
        sync_and_close();
    }
private:
    std::atomic<bool> m_stop;
    thread_logqueue m_head;
    std::thread m_logging_thread;
};

}

#endif