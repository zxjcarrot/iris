#include <cstdio>
#include <cstdlib>
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

template<typename T>
static void clear_pointer_vector(std::vector<T*> & vec) {
    for (size_t i = 0; i < vec.size(); ++i) {
        delete vec[i];
    }
    vec.clear();
}

static void iris_thread(writer * pwriter, std::atomic<bool> * stop, thread_logqueue * head) {
    this_thread_logqueue.reset(new thread_logqueue(head));
    std::vector<loglet_t *> logs;
    std::vector<struct iovec> iovecs;
    std::vector<buffer *> log_buffers;
    while(!*stop) {
        // iterate through the linked list of input queues, collect log entries
        thread_logqueue * p = head;
        clear_pointer_vector(logs);
        while (p) {
            p->q.batch_poll(logs);
            p = p->next;
        }

        if (logs.empty()) {
            std::this_thread::yield();
        } else {
            iovecs.clear();
            for (size_t i = 0; i < logs.size(); ++i)  {
                if (iovecs.size() <= i) {
                    struct iovec vec;
                    iovecs.push_back(vec);
                }
                if (log_buffers.size() <= i) {
                    log_buffers.push_back(new buffer());
                }
                buffer * pbuf = log_buffers[i];
                auto f = *reinterpret_cast<formatter_t**>(logs[i]->buf.buf);
                pbuf->reset();
                (*f)(logs[i], *pbuf);
                iovecs[i].iov_base = pbuf->buf;
                iovecs[i].iov_len = pbuf->size();
            }

            pwriter->write(iovecs);
        }
    }
    clear_pointer_vector(log_buffers);
}

logger::logger(level l, writer * pwriter) : m_stop(false) {
    if (pwriter == nullptr)
        pwriter = &stdout_writer;
    m_logging_thread = std::thread(iris_thread, pwriter, &m_stop, &m_head);
    log_level = l;
}