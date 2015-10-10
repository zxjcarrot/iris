#ifndef IRIS_BASE_LOG_H
#define IRIS_BASE_LOG_H

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
#include "lfringbuffer.h"
#include "sslfqueue.h"
#include "notifier.h"
#include "utils.h"
#include "formatter.h"

namespace iris {

struct thread_logqueue;
struct thread_logqueue_holder;
extern notifier ntfer;
extern thread_logqueue_holder this_thread_logqueue;
extern size_t thread_queue_size;
extern size_t thread_ringbuf_size;

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
public:
    thread_logqueue(size_t queue_size = 5000, size_t rbuf_size = 102400); // used only by normal threadss
    thread_logqueue(thread_logqueue *head, size_t queue_size = 5000, size_t rbuf_size = 102400); // used only by output thread
    ~thread_logqueue();
public:
    sslfqueue<loglet_t>             q;
    lfringbuffer                    rbuf;
    std::atomic<thread_logqueue *>  next;
    thread_logqueue               * head;
    bool                            output_thread; // if current thread is the output thread
};

class base_logger {
public:
    base_logger(writer * pwriter = nullptr,
                size_t scan_interval = 100,
                size_t output_buffer_size = 102400,
                size_t default_thread_ringbuf_size = 102400,
                size_t default_thread_queue_size = 5000);

    // set the thread local queue size
    void set_thread_queue_size(size_t size) {
        thread_queue_size = size;
    }
    // set the thread local ring buffer size
    void set_thread_ringbuf_size(size_t size) {
        thread_ringbuf_size = size;
    }

    template<typename Formatter, typename... Args>
    void log(Args&&... args) {
        thread_logqueue * this_queue = this_thread_logqueue.get();

        if (iris_unlikely(!this_queue)) {
            if (thread_queue_size == 0)
                thread_queue_size = m_default_thread_queue_size;
            if (thread_ringbuf_size == 0)
                thread_ringbuf_size = m_default_thread_ringbuf_size;
            this_queue = new thread_logqueue(&m_head, thread_queue_size, thread_ringbuf_size);
            this_thread_logqueue.assign(this_queue);
        }

        typedef std::tuple<typename std::decay<Args>::type...> Args_t;
        const size_t buffer_size = sizeof(formatter_type) + sizeof(Args_t); 
        const size_t args_offset = sizeof(formatter_type);

        loglet_t l;
        while ((l.rbuf_alloc_size = this_queue->rbuf.acquire(buffer_size, l.rbuf_ptr)) == 0)
            std::this_thread::yield();
        *reinterpret_cast<formatter_type*>(l.rbuf_ptr) = &formatter_caller<Formatter, typename std::decay<Args>::type...>;
        // inplace construct parameter pack
        new (l.rbuf_ptr + args_offset) Args_t(std::forward<Args>(args)...);

        // try to publish to output thread
        if (iris_likely(this_queue->q.offer(l))) {
            return;
        }

        // queue is full, notify the output thread
        ntfer.notify(notifier::to_ntf_t(reinterpret_cast<long>(this_queue), ntf_msg));
        // busy yielding until the log is buffered
        do {
            std::this_thread::yield();
        } while (!this_queue->q.offer(l));
    }

    void sync_and_close() {
        if (!m_stop.load(std::memory_order_acquire)) {
            this_thread_logqueue.notify_and_quit();
            m_stop.store(true, std::memory_order_release);
            m_output_thread.join();
        }
    }

    ~base_logger() {
        sync_and_close();
    }
private:
    std::atomic<bool> m_stop;
    thread_logqueue m_head;
    std::thread m_output_thread;
    size_t m_default_thread_ringbuf_size;
    size_t m_default_thread_queue_size;
};

}

#endif
