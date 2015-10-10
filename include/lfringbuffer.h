#ifndef IRIS_LF_RINGBUFFER_H_
#define IRIS_LF_RINGBUFFER_H_
#include <atomic>

#include "define.h"
#include "utils.h"
#include <queue>
#include <assert.h>
namespace iris {

// A single reader single writer lockfree ring buffer
// this should be used as following:
// A thread calls acquire() method repeatedly 
// until the return value is nonzero to allocate @size bytes of memory.
// And release() method should be called in the order of allocation 
// with the same @size parameter to declare the release.
class lfringbuffer {
private:
    size_t                      cap;
    size_t                      mask;
    char                     *  buffer;
    char                        __pad0__[IRIS_CACHELINE_SIZE - sizeof(size_t) * 2 - sizeof(char*)];
    unsigned long               head;
    unsigned long               tail_cache;
    char                        __pad1__[IRIS_CACHELINE_SIZE - sizeof(unsigned long) * 2];
    std::atomic<unsigned long>  tail;
    char                        __pad2__[IRIS_CACHELINE_SIZE - sizeof(std::atomic<unsigned long>)];
public:
    lfringbuffer(size_t size) {
        cap = round_up_to_next_multiple_of_2(size);
        mask = cap - 1;
        buffer = new char[cap];
        head = 0;
        tail = cap;
        tail_cache = cap;
    }

    ~lfringbuffer() {
        delete[] buffer;
        buffer = nullptr;
    }

    // acquire() tries to allocate @size bytes of memory
    // from the buffer, the address of the memory acquired
    // is stored in @ptr.
    // returns the actual bytes allocated insided the buffer 
    // which might be larger than the requested size, since
    // internally the buffer is implemented by a array and 
    // there is unusable memory need to be taken into account
    // when the request can not be fulfilled using only the memory
    // left at the end of the buffer.
    // 0 will be returned if there is not enough free space.
    size_t acquire(size_t size, char *& ptr) {
        assert(size);
        size_t total_free = tail_cache - head;
        if (iris_unlikely(total_free < size)) {
            // load the latest tail
            tail_cache = tail.load(std::memory_order_acquire);
            total_free = tail_cache - head;
            if (total_free < size)
                return 0; // no enough freespace
        }

        // now check if there is enough memory at the rear
        size_t rear_free = cap - (head & mask);
        if (iris_likely(rear_free >= size)) {
            // ok, take the rear chunk
            ptr = buffer + (head & mask);
            head += size;
            return size;
        }

        // now check if there is enough memory at the front
        size_t front_free = tail_cache & mask;
        if (front_free < size) {
            tail_cache = tail.load(std::memory_order_acquire);
            front_free = tail_cache & mask;
            if (front_free < size)
                return 0;// no enough space
        }

        ptr = buffer;

        // throw away the rear fragmentation
        head += rear_free + size;

        return rear_free + size;
    }

    // release() method releases @size bytes of memory allocated from
    // acquire() method. @size should be the same as the one retuned from
    // acquire().
    void   release(size_t size) {
        tail.fetch_add(size, std::memory_order_release);
    }

    size_t freespace() {
        return tail.load(std::memory_order_acquire) - head;
    }
};

}
#endif