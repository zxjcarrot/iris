#ifndef IRIS_SSLFQUEUE_H_
#define IRIS_SSLFQUEUE_H_
#include <stdint.h>
#include <atomic>

namespace iris {

//a single producer single consumer lockfree ring queue.
template<typename T, int CAP>
class sslfqueue {
private:
	int                 cap;
	int                 mask;
    T *                 buffer;
	char                _pad3_[64 - sizeof(T*) - sizeof(int) * 2];
    std::atomic<long>   head;
	char                _pad4_[64 - sizeof(std::atomic<long>)];
	long		        tail_cache;
	char                _pad5_[64 - sizeof(long)];
    std::atomic<long>   tail;
	char                _pad6_[64 - sizeof(std::atomic<long>)];
	long		        head_cache;
	char                _pad7_[64 - sizeof(long)];

	inline static int next_multiple_of_2(int n) {
		int bits = 0;
		while(n) {
			++bits;
			n >>= 1;
		}

		return 1 << bits;
	}

public:

    sslfqueue():head(0), tail_cache(0), tail(0), head_cache(0)
    {
    	cap = next_multiple_of_2(CAP);
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
            cur_head++;
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
};

}
#endif


