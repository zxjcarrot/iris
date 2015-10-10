#ifndef IRIS_SSLFQUEUE_H_
#define IRIS_SSLFQUEUE_H_
#include <stdint.h>
#include <atomic>
#include <vector>
#include "utils.h"

namespace iris {

//a single producer single consumer lockfree ring queue.
template<typename T>
class sslfqueue {
private:
	size_t              cap;
	size_t              mask;
    T *                 buffer;
	char                _pad0_[IRIS_CACHELINE_SIZE - sizeof(T*) - sizeof(int) * 2];
    std::atomic<long>   head;
	char                _pad1_[IRIS_CACHELINE_SIZE - sizeof(std::atomic<long>)];
	long		        tail_cache;
	char                _pad2_[IRIS_CACHELINE_SIZE - sizeof(long)];
    std::atomic<long>   tail;
	char                _pad3_[IRIS_CACHELINE_SIZE - sizeof(std::atomic<long>)];
	long		        head_cache;
	char                _pad4_[IRIS_CACHELINE_SIZE - sizeof(long)];

public:

    sslfqueue(size_t capacity = 5000):head(0), tail_cache(0), tail(0), head_cache(0)
    {
    	cap = round_up_to_next_multiple_of_2(capacity);
    	mask = cap - 1;
		buffer = new T[cap];
	}

	~sslfqueue(){
		delete []buffer;
	}

    bool offer(const T & e) {
        const long cur_tail = tail.load(std::memory_order_acquire);
        const long len = cur_tail - cap;
        if (head_cache <= len) {
			head_cache = head.load(std::memory_order_acquire);
			if (head_cache <= len)
				return false; // queue is full
        }

        buffer[cur_tail & mask] = e;
        tail.store(cur_tail + 1, std::memory_order_release);

        return true;
    }

    bool poll(T & e) {
        const long cur_head = head.load(std::memory_order_acquire);
        if (cur_head >= tail_cache) {
			tail_cache = tail.load(std::memory_order_acquire);
			if (cur_head >= tail_cache)
				return false; // queue is empty
        }

        e = buffer[cur_head & mask];
        head.store(cur_head + 1, std::memory_order_release);

        return true;
    }

    bool batch_poll(std::vector<T> & vec) {
        long cur_head = head.load(std::memory_order_acquire);
        if (cur_head >= tail_cache) {
            tail_cache = tail.load(std::memory_order_acquire);
            if (cur_head >= tail_cache)
                return false; // queue is empty
        }

        while (cur_head < tail_cache) {
            vec.push_back(buffer[cur_head & mask]);
            ++cur_head;
        }

        head.store(cur_head, std::memory_order_release);

        return true;
    }

    bool empty() {
        return tail.load(std::memory_order_acquire) - head.load(std::memory_order_acquire) == 0;
    }

    bool full() {
        return tail.load(std::memory_order_acquire) - head.load(std::memory_order_acquire) == cap;
    }

    size_t size() {
        return tail.load(std::memory_order_acquire) - head.load(std::memory_order_acquire);
    }
};

}
#endif


