
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




enum data_type{
    TYPE_BOOL = 0,
    TYPE_UCHAR = 1,
    TYPE_CHAR = 2,
    TYPE_USHORT = 3,
    TYPE_SHORT = 4,
    TYPE_UINT  = 5,
    TYPE_INT   = 6,
    TYPE_ULONG = 7,
    TYPE_LONG = 8,
    TYPE_ULLONG = 9,
    TYPE_LLONG = 10,
    TYPE_FLOAT = 11,
    TYPE_DOUBLE = 12,
    TYPE_LONGDOUBLE = 13,
    TYPE_VOIDADDR = 14,
    TYPE_CHARADDR = 15,
    TYPE_STRING  = 16
};

struct data {
    enum data_type type;
    union {
        void *          addr;
        bool            b;
        char            c;
        short           s;
        int             i;
        long int        l;
        long long int   ll;
        float           f;
        double          d;
        long double     ld;
    };
    std::string     str;
};

struct loglet_t {
    const char *                            fmt;
    std::vector<data>                       args;  
};

typedef std::shared_ptr<loglet_t> sp_loglet_t;
struct thread_logqueue;
extern std::unique_ptr<thread_logqueue> this_thread_logqueue;
extern level log_level;

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
        //printf(fmt, args...);
        //printf("\n");
        sp_loglet_t sl(new loglet_t);
        sl->fmt = fmt;
        append(sl, args...);
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
    template<typename... Arg>
    void append(sp_loglet_t &sl) {}

    template<typename... Args>
    void append(sp_loglet_t & sl, bool arg, Args... args) {
        struct data d;
        d.type = TYPE_BOOL;
        d.c = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, unsigned char arg, Args... args) {
        struct data d;
        d.type = TYPE_UCHAR;
        d.c = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, char arg, Args... args) {
        struct data d;
        d.type = TYPE_CHAR;
        d.c = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, unsigned short arg, Args... args) {
        struct data d;
        d.type = TYPE_USHORT;
        d.s = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, short arg, Args... args) {
        struct data d;
        d.type = TYPE_SHORT;
        d.s = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, unsigned int arg, Args... args) {
        struct data d;
        d.type = TYPE_UINT;
        d.i = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, int arg, Args... args) {
        struct data d;
        d.type = TYPE_INT;
        d.i = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, unsigned long arg, Args... args) {
        struct data d;
        d.type = TYPE_ULONG;
        d.l = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, long arg, Args... args) {
        struct data d;
        d.type = TYPE_LONG;
        d.l = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, unsigned long long arg, Args... args) {
        struct data d;
        d.type = TYPE_ULLONG;
        d.ll = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, long long arg, Args... args) {
        struct data d;
        d.type = TYPE_LLONG;
        d.ll = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, float arg, Args... args) {
        struct data d;
        d.type = TYPE_FLOAT;
        d.f = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, double arg, Args... args) {
        struct data d;
        d.type = TYPE_DOUBLE;
        d.d = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, long double arg, Args... args) {
        struct data d;
        d.type = TYPE_LONGDOUBLE;
        d.ld = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, unsigned char * arg, Args... args) {
        append(sl, (char *)arg, args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, char * arg, Args... args) {
        struct data d;
        d.type = TYPE_CHARADDR;
        d.addr = arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename T, typename... Args>
    void append(sp_loglet_t & sl, T * arg, Args... args) {
        struct data d;
        d.type = TYPE_VOIDADDR;
        d.addr = (void*)arg;
        sl->args.push_back(std::move(d));
        append(sl,args...);
    }

    template<typename... Args>
    void append(sp_loglet_t & sl, const std::string & arg, Args... args) {
        struct data d;
        d.str = arg;
        sl->args.push_back(std::move(d));
        sl->args.back().type = TYPE_STRING;
        append(sl,args...);
    }
private:
    std::atomic<bool> m_stop;
    thread_logqueue m_head;
    std::thread m_logging_thread;
};

}

#endif