
#ifndef IRIS_LOG_H_
#define IRIS_LOG_H_

#include <cmath>
#include <cstdio>

#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <string>
#include <utility>
#include <string.h>

#include "define.h"
#include "writer.h"
#include "sslfqueue.h"
#include "notifier.h"
#include "utils.h"

namespace iris {

struct log_info_t {
    const char   *  fmt;
    struct timeval  ts;
    std::thread::id tid;
    log_info_t(const char * format_str);
};

struct loglet_t {
    const char                            * fmt;
    // @pbuffer holds formatter function address and arguments
    buffer                                  buf;
    loglet_t(const char * format_str, size_t buffer_size = 0):fmt(format_str), buf(buffer_size){}
};


// fomatter do the real formatting logic, output into @obuf
// return false if there is not enough space in @obuf.
typedef bool formatter_t(loglet_t * l, buffer & obuf);

struct thread_logqueue;
struct thread_logqueue_holder;
extern level log_level;
extern notifier ntfer;
extern thread_logqueue_holder this_thread_logqueue;
extern long long last_timestamp; // timestamp of last successful log publication

struct thread_logqueue_holder {
private:
    thread_logqueue * q;
public:
    thread_logqueue_holder():q(nullptr){}
    inline void assign(thread_logqueue * queue) {
        q = queue;
    }
    inline thread_logqueue *get() {
        return q;
    }
    inline void notify_and_quit() {
        if (q) {
            ntfer.notify(notifier::to_ntf_t((long)q, ntf_queue_deletion));
            q = nullptr;
        }
    }
    ~thread_logqueue_holder() {
        notify_and_quit();
    }
};

// Every thread which uses logging primitives owns one of this 
// single-wrtiter-single-reader queue for buffering logs.
// These thread_local queues form a singly-linked list which is
// polled by a dedicated output thread to collect logs.
struct thread_logqueue {
    thread_logqueue();                      // used only by normal threadss
    thread_logqueue(thread_logqueue *head); // used only by output thread
    ~thread_logqueue();
    sslfqueue<loglet_t*, 1024000> q;
    std::atomic<thread_logqueue *> next;
    thread_logqueue * head;
    bool output_thread;                    // if current thread is the output thread
};

template<size_t ...> struct seq {};
template<size_t idx, std::size_t N, std::size_t... S> struct seq_helper: seq_helper<idx + 1, N, S..., idx> {};
template<size_t N, size_t ...S> struct seq_helper<N, N, S...> {
    typedef seq<S...> type;
};
template<size_t N>
struct make_sequence {
    typedef typename seq_helper<0, N>::type type;
};

template<typename...Args, std::size_t...Indexes>
static int call_snprintf(buffer & obuf,const char * fmt, const std::tuple<Args...> & args, seq<Indexes...>) {
    return std::snprintf(obuf.write_pointer(), obuf.freespace(), fmt, std::get<Indexes>(args)...);
}

template<typename... Args>
static bool formatter(loglet_t * l, buffer & obuf) {
    size_t args_offset = sizeof(formatter_t*), n = obuf.size();
    typedef std::tuple<Args...> Args_t;
    Args_t & args = *reinterpret_cast<Args_t*>(l->buf.buf + args_offset);

    while (true) {
        typename make_sequence<sizeof...(Args)>::type indexes;
        n = call_snprintf(obuf, l->fmt, args, indexes);
        if (iris_unlikely(n < 0)) {
            break; //skip over this formatting if error occurred
        }
        if (iris_unlikely(n >= obuf.freespace())) {
            return false;
        }
        obuf.pos += n;
        obuf[obuf.pos++] = '\n'; // replace '\0' with '\n'
        break; // TODO what to do with failed formatting?
    }
    //deconstruct parameter pack
    args.~Args_t();
    return true;
}

class logger {
public:
    logger(size_t notify_interval = 10000, size_t output_buffer_size = 102400, level l = INFO, writer * pwriter = nullptr);

    void set_level(level l) {
        log_level = l;
    }

    template<typename... Args>
    void log(level lvl, const char * fmt, Args&&... args) {
        if (lvl < log_level)
            return;

        if (iris_unlikely(!this_thread_logqueue.get())) {
            this_thread_logqueue.assign(new thread_logqueue(&m_head));
        }
        typedef std::tuple<typename std::decay<Args>::type...> Args_t;
        size_t buffer_size = sizeof(formatter_t*) + sizeof(Args_t);
        size_t args_offset = sizeof(formatter_t*);
        loglet_t * l = new loglet_t(fmt, buffer_size);

        *reinterpret_cast<formatter_t**>(l->buf.buf) = &formatter<typename std::decay<Args>::type...>;
        // inplace construct parameter pack
        new (l->buf.buf + args_offset) Args_t(std::forward<Args>(args)...);
        l->buf.pos += buffer_size;

        // try to publish to output thread
        if (this_thread_logqueue.get()->q.offer(l)) {
            long long time_in_us = get_current_time_in_us();
            if (time_in_us - last_timestamp >= m_notify_interval) {
                ntfer.notify(notifier::to_ntf_t(reinterpret_cast<long>(this_thread_logqueue.get()), ntf_msg));
            }
            last_timestamp = time_in_us;
            return;
        }
        // it queue is full, notify the output thread
        ntfer.notify(notifier::to_ntf_t(reinterpret_cast<long>(this_thread_logqueue.get()), ntf_msg));
        // busy yielding until the log is buffered
        do {
            std::this_thread::yield();
        } while (!this_thread_logqueue.get()->q.offer(l));
    }

    template<typename... Args>
    void trace(const char * fmt, Args&&... args) {
        log(TRACE, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const char * fmt, Args&&... args) {
        log(DEBUG, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const char * fmt, Args&&... args) {
        log(INFO, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const char * fmt, Args&&... args) {
        log(WARN, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const char * fmt, Args&&... args) {
        log(ERROR, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(const char * fmt, Args&&... args) {
        log(FATAL, fmt, std::forward<Args>(args)...);
    }

    void sync_and_close() {
        if (!m_stop.load(std::memory_order_acquire)) {
            this_thread_logqueue.notify_and_quit();
            m_stop.store(true, std::memory_order_release);
            m_output_thread.join();
        }
    }

    ~logger() {
        sync_and_close();
    }
private:
    std::atomic<bool> m_stop;
    thread_logqueue m_head;
    std::thread m_output_thread;
    size_t m_notify_interval;
};

}

#endif
