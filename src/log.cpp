#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <memory>
#include <log.h>
#include <stream_writer.h>

using namespace iris;

namespace iris{

static stream_writer stdout_writer(stdout);

thread_logqueue_holder this_thread_logqueue; 
notifier ntfer;
level log_level;
long long last_timestamp;
}

thread_logqueue::thread_logqueue(): next(nullptr),output_thread(true) 
    {}

thread_logqueue::thread_logqueue(thread_logqueue *head): next(nullptr), head(head), output_thread(false) {
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
    if (output_thread)
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

static void iris_thread(writer * pwriter, std::atomic<bool> * stop, thread_logqueue * head, size_t buffer_size) {
    this_thread_logqueue.assign(new thread_logqueue(head));
    std::vector<loglet_t *> logs;
    std::vector<buffer *> log_buffers;
    std::vector<ntf_t>    ntfs;
    buffer buf(buffer_size);
    while(!*stop) {
        // iterate through the linked list of input queues, collect log entries
        thread_logqueue * p = head;
        clear_pointer_vector(logs);
        while (p) {
            p->q.batch_poll(logs);
            p = p->next;
        }

        if (logs.empty()) {
            ntfs.clear();
            // wait for notification or 100ms
            ntfer.wait(100, ntfs);
            for (size_t i = 0; i < ntfs.size(); ++i) {
                ntf_t ntf = ntfs[i];
                p = reinterpret_cast<thread_logqueue *>(notifier::to_data_t(ntf));
                switch(notifier::to_ntf_type(ntf)) {
                    case ntf_msg:
                        p->q.batch_poll(logs);
                    break;
                    case ntf_queue_deletion:
                        p->q.batch_poll(logs);
                        delete p;
                    break;
                    default:
                    break;
                }
            }
        }
        for (size_t i = 0; i < logs.size(); ++i) {
            auto f = *reinterpret_cast<formatter_t**>(logs[i]->buf.buf);
            if ((*f)(logs[i], buf) == false) {
                --i; // offset with ++i, redo the formatting again.
                pwriter->write(buf, buf.size());
                buf.reset();
            }
        }
        if (buf.size()) {
            pwriter->write(buf.buf, buf.size());
            buf.reset();
        }
    }

    //collect one more time
    thread_logqueue * p = head;
    clear_pointer_vector(logs);
    while (p) {
        p->q.batch_poll(logs);
        p = p->next;
    }
    for (size_t i = 0; i < logs.size(); ++i) {
        auto f = *reinterpret_cast<formatter_t**>(logs[i]->buf.buf);
        if ((*f)(logs[i], buf) == false) {
            --i; // offset with ++i, redo the formatting again.
            pwriter->write(buf.buf, buf.size());
            buf.reset();
        }
    }
    if (buf.size()) {
        pwriter->write(buf.buf, buf.size());
        buf.reset();
    }
}

logger::logger(size_t notify_interval, size_t output_buffer_size, level l, writer * pwriter): m_notify_interval(notify_interval), m_stop(0) {
    if (pwriter == nullptr)
        pwriter = &stdout_writer;
    m_output_thread = std::thread(iris_thread, pwriter, &m_stop, &m_head, output_buffer_size);
    log_level = l;
}