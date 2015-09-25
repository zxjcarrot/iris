#include <cstdio>
#include <cmath>

#include <memory>

#include <log.h>
#include <stream_writer.h>

#include "format_extractor.h"
using namespace iris;

namespace iris{

static stream_writer stdout_writer(stdout);

std::unique_ptr<thread_logqueue> this_thread_logqueue; 
level log_level;
}

thread_logqueue::thread_logqueue(): next(nullptr),logging_thread(true) 
    {}


thread_logqueue::thread_logqueue(thread_logqueue *head): next(nullptr), head(head), logging_thread(false) {
    thread_logqueue * p = nullptr;

    do {
        // insert self into the head of the global linked list lockfreely
        p = head->next.load(std::memory_order_acquire);
        this->next.store(p, std::memory_order_release);
        // head might have been modified or deleted cas until this is inserted
        if (head->next.compare_exchange_weak(p, this, std::memory_order_release)) {
            return;
        }
    } while (true);
}

thread_logqueue::~thread_logqueue() {
    if (logging_thread)
        return;
    thread_logqueue * p = nullptr, *pnext = nullptr, *q = this;

    // remove self from the global linked list lockfreely
    p = head;
    while (p->next.load(std::memory_order_acquire) != q)
        p = p->next.load(std::memory_order_acquire);

    pnext = this->next.load(std::memory_order_acquire);
    // mark this as deleted(by setting this->next to nullptr)
    while (!this->next.compare_exchange_weak(pnext, nullptr, std::memory_order_release)) {
        next = this->next.load(std::memory_order_acquire);
    }

    do {
        if (p->next.compare_exchange_weak(q, pnext, std::memory_order_release)) {
            return;
        }
        // some other nodes have been inserted after p, restart.
        p = head;
        while (p->next.load(std::memory_order_acquire) != q)
            p = p->next.load(std::memory_order_acquire);
    } while(true);
}


struct log_buffer {
    log_buffer(size_t cap = 0):buffer(nullptr), pos(0), capacity(cap){
        if (capacity) {
            buffer = new char[capacity];
        }
    }

    ~log_buffer() {
        if (buffer) {
            delete[] buffer;
            buffer = nullptr;
            pos = capacity = 0;
        }
    }

    inline void reserve(size_t n) {
        if (n <= capacity) 
            return;
        //log_buffer tmp(n);
        //std::swap(tmp.buffer, this->buffer);
        delete[] this->buffer;
        this->buffer = new char[n];
        this->capacity = n;
    }

    inline void reset() {
        pos = 0;
    }

    inline size_t freespace() {
        return capacity - pos;
    }

    char * write_pointer() {
        return buffer + pos;
    }

    inline size_t size() {
        return pos;
    }

    inline operator char*() {
        return buffer;
    }

    char      *buffer;
    size_t     pos;
    size_t     capacity;
};

static void format_log (const sp_loglet_t & sl, log_buffer & buf) {
    size_t len = strlen(sl->fmt) + sl->args.size() * 5;
    size_t idx = 0;
    int type;
    char *start, *end;
    format_extractor extarctor(sl->fmt);

    buf.reserve(len);
    buf.reset();

    while (extarctor.extract(type, start, end) && idx < sl->args.size()) {
        bool need_expand = false;
        switch (type) {
            case TYPE_OTHER:
                buf.reserve(buf.size() + (end - start) + 1);
                while (start <= end) {
                    buf[buf.pos++] = *start++;
                }
            break;
            case TYPE_SPECIFIER:
                std::string fmt(start, end + 1);
                size_t n;
                struct data & d = sl->args[idx];
                while (true) {
                    switch(d.type) {
                        case TYPE_BOOL:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (bool)d.b);
                        break;
                        case TYPE_UCHAR:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (unsigned char)d.c);
                        break;
                        case TYPE_CHAR:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (char)d.c);
                        break;
                        case TYPE_USHORT:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (unsigned short)d.s);
                        break;
                        case TYPE_SHORT:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (short)d.s);
                        break;
                        case TYPE_UINT:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (unsigned int)d.i);
                        break;
                        case TYPE_INT:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (int)d.i);
                        break;
                        case TYPE_ULONG:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (unsigned long)d.l);
                        break;
                        case TYPE_LONG:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (long)d.l);
                        break;
                        case TYPE_ULLONG:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (unsigned long long)d.ll);
                        break;
                        case TYPE_LLONG:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (long long)d.ll);
                        break;
                        case TYPE_FLOAT:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (float)d.f);
                        break;
                        case TYPE_DOUBLE:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (double)d.d);
                        break;
                        case TYPE_LONGDOUBLE:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (long double)d.ld);
                        break;
                        case TYPE_VOIDADDR:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (void*)d.addr);
                        break;
                        case TYPE_CHARADDR:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), (const char*)d.addr);
                        break;
                        case TYPE_STRING:
                            n = snprintf(buf.write_pointer(), buf.freespace(), fmt.c_str(), d.str.c_str());
                        break;
                    }
                    if (n >= buf.freespace()) {
                        buf.reserve(buf.size() + n + 1);
                    } else {
                        if (n > 0) {
                            buf.pos += n;
                        }
                        idx++; // we skip over this argument not matter it succeeded or failed to format;
                        break;
                    }
                }
            break;
        }
    }
    buf[buf.pos++] = '\n';
}

static void format_logs (const std::vector<sp_loglet_t> & logs, std::vector<std::shared_ptr<log_buffer>> & log_buffers) {
    for (size_t i = 0; i < logs.size(); ++i) {
        if (log_buffers.size() <= i) {
            log_buffers.push_back(std::shared_ptr<log_buffer>(new log_buffer()));
        }
        format_log(logs[i], *log_buffers[i]);
    }
}

static void iris_thread(writer * pwriter, std::atomic<bool> * stop, thread_logqueue * head) {
    this_thread_logqueue.reset(new thread_logqueue(head));
    std::vector<sp_loglet_t> logs;
    std::vector<struct iovec> iovecs;
    std::vector<std::shared_ptr<log_buffer>> log_buffers;
    struct iovec vec;
    while(!*stop) {
        // iterate through linked list, collect log entries
        thread_logqueue * p = head;
        logs.clear();
        while (p) {
            p->q.batch_poll(logs);
            p = p->next;
        }

        if (logs.empty()) {
            std::this_thread::yield();
        } else {
            format_logs(logs, log_buffers);

            iovecs.clear();
            for (size_t i = 0; i < logs.size(); ++i)  {
                if (iovecs.size() <= i) {
                    iovecs.push_back(vec);
                }
                iovecs[i].iov_base = *log_buffers[i];
                iovecs[i].iov_len = log_buffers[i]->pos;
            }

            pwriter->write(iovecs);
        }
    }
}

logger::logger(level l, writer * pwriter) : m_stop(false) {
    if (pwriter == nullptr)
        pwriter = &stdout_writer;
    m_logging_thread = std::thread(iris_thread, pwriter, &m_stop, &m_head);
    log_level = l;
}