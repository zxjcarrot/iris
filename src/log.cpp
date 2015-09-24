#include <cstdio>
#include <cmath>

#include <mutex>

#include <log.h>
#include <stream_writer.h>

using namespace iris;

namespace iris{

static stream_writer stdout_writer(stdout);

thread_local std::unique_ptr<thread_logqueue> this_thread_logqueue;
thread_local level log_level;
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

static void iris_thread(writer * pwriter, std::atomic<bool> * stop, thread_logqueue * head) {
    this_thread_logqueue.reset(new thread_logqueue(head));
    std::vector<sp_loglet_t> logs;
    std::vector<struct iovec> iovecs;
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
            iovecs.clear();
            for (auto & sl : logs) {
                vec.iov_base = &(*sl)[0];
                vec.iov_len = sl->size();
                iovecs.push_back(vec);
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