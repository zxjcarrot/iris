#include <cstdio>
#include <cstdlib>
#include <cmath>

#include <memory>
#include <base_logger.h>
#include <stream_writer.h>

using namespace iris;

namespace iris{

static stream_writer stdout_writer(stdout);

thread_logqueue_holder this_thread_logqueue; 
notifier ntfer;
size_t thread_queue_size;
size_t thread_ringbuf_size;

}

thread_logqueue::thread_logqueue(size_t queue_size, size_t rbuf_size): q(queue_size), rbuf(rbuf_size), next(nullptr), output_thread(true)
    {}

thread_logqueue::thread_logqueue(thread_logqueue *head, size_t queue_size, size_t rbuf_size): q(queue_size), rbuf(rbuf_size), next(nullptr), head(head), output_thread(false){
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

static void do_format_and_flush(thread_logqueue *p, std::vector<loglet_t> & logs, buffered_writer & w) {
    for (size_t i = 0; i < logs.size(); ++i) {
        auto f = *reinterpret_cast<formatter_type*>(logs[i].rbuf_ptr);
        (*f)(logs[i], w);
        p->rbuf.release(logs[i].rbuf_alloc_size);
    }
    logs.clear();
}

static void iris_thread(writer * pwriter, std::atomic<bool> * stop, size_t scan_interval,size_t output_buffer_size, thread_logqueue * head) {
    std::vector<loglet_t> logs;
    std::vector<ntf_t>    ntfs;
    buffered_writer bw(*pwriter, output_buffer_size);
    while(!*stop) {
        // iterate through the linked list of input queues, collect log entries
        thread_logqueue * p = head;
        bool empty = true;
        while (p) {
            if (p->q.batch_poll(logs)) {
                do_format_and_flush(p, logs, bw);
                empty = false;
            }
            p = p->next;
        }

        if (empty) {
            ntfs.clear();
            // wait for notification or 100ms
            ntfer.wait(scan_interval, ntfs);
            for (size_t i = 0; i < ntfs.size(); ++i) {
                ntf_t ntf = ntfs[i];
                p = reinterpret_cast<thread_logqueue *>(notifier::to_data_t(ntf));
                switch(notifier::to_ntf_type(ntf)) {
                    case ntf_msg:
                        if (p->q.batch_poll(logs))
                            do_format_and_flush(p, logs, bw);
                    break;
                    case ntf_queue_deletion:
                        if (p->q.batch_poll(logs))
                            do_format_and_flush(p, logs, bw);
                        delete p;
                    break;
                    default:
                    break;
                }
            }
        }
    }

    //collect one more time
    thread_logqueue * p = head;
    while (p) {
        if (p->q.batch_poll(logs))
            do_format_and_flush(p, logs, bw);
        p = p->next;
    }
    bw.flush();
}

base_logger::base_logger(writer * pwriter,
                size_t scan_interval,
                size_t output_buffer_size,
                size_t default_thread_ringbuf_size,
                size_t default_thread_queue_size):
    m_stop(0),
    m_default_thread_ringbuf_size(default_thread_ringbuf_size),
    m_default_thread_queue_size(default_thread_queue_size)
{
    if (pwriter == nullptr)
        pwriter = &stdout_writer;
    m_output_thread = std::thread(iris_thread, pwriter, &m_stop, scan_interval, output_buffer_size, &m_head);
}
