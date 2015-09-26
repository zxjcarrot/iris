
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

#include "define.h"
#include "writer.h"
#include "sslfqueue.h"

namespace iris {

typedef std::shared_ptr<loglet_t> sp_loglet_t;
typedef void formatter_t(loglet_t * l, buffer & obuf);
struct thread_logqueue;
extern std::unique_ptr<thread_logqueue> this_thread_logqueue;
extern level log_level;

struct thread_logqueue {
    thread_logqueue();                      // used only by normal threads
    thread_logqueue(thread_logqueue *head); // used only by logging thread
    ~thread_logqueue();
    sslfqueue<loglet_t*, 5000> q;
    std::atomic<thread_logqueue *> next;
    thread_logqueue * head;
    bool logging_thread;                 // if current thread is the logging thread
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
static int call_snprintf(buffer & obuf,const char * fmt, std::tuple<Args...> args, seq<Indexes...>) {
    return std::snprintf(obuf.write_pointer(), obuf.freespace(), fmt, std::get<Indexes>(args)...);
}
template<typename... Args>
static void formatter(loglet_t * l, buffer & obuf) {
    size_t args_offset = sizeof(formatter_t*), n = obuf.size();
    typedef std::tuple<Args...> Args_t;
    Args_t & args = *reinterpret_cast<Args_t*>(l->buf.buf + args_offset);

    obuf.expand(obuf.size() + strlen(l->fmt) + sizeof...(Args) * 5);
    while (true) {
        typename make_sequence<sizeof...(Args)>::type indexes;
        n = call_snprintf(obuf, l->fmt, args, indexes);
        if (n > obuf.freespace()) {
            obuf.expand(obuf.size() + n + 2);
        } else {
            if (n > 0) {
                obuf.pos += n;
            }
            break;
        }
    }

    obuf[obuf.pos++] = '\n';
    //deconstruct parameter pack
    args.~Args_t();
}

class logger {
public:
    logger(level l = INFO, writer * pwriter = nullptr);

    void set_level(level l) {
        log_level = l;
    }

    template<typename... Args>
    void log(level lvl, const char * fmt, Args&&... args) {
        if (lvl < log_level)
            return;

        if (iris_unlikely(!this_thread_logqueue.get())) {
            this_thread_logqueue = std::unique_ptr<thread_logqueue>(new thread_logqueue(&m_head));
        }
        typedef std::tuple<typename std::decay<Args>::type...> Args_t;
        size_t buffer_size = sizeof(formatter_t*) + sizeof(Args_t);
        size_t args_offset = sizeof(formatter_t*);
        loglet_t * l = new loglet_t(fmt, buffer_size);

        *reinterpret_cast<formatter_t**>(l->buf.buf) = &formatter<typename std::decay<Args>::type...>;
        //inplace construct parameter pack
        new (l->buf.buf + args_offset) Args_t(std::forward<Args>(args)...);
        l->buf.pos += buffer_size;

        while(!this_thread_logqueue->q.offer(l))
            std::this_thread::yield();
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