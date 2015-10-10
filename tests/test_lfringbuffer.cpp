#include <thread>
#include <string>

#include <assert.h>

#include <lfringbuffer.h>
#include <sslfqueue.h>
#include <define.h>


using namespace iris;
#define ITERATIONS (int)1e7
lfringbuffer rbuf(1024);
struct buffer_t {
    char * b;
    int    size;
    int    alloc_size;
    int    data;
};
sslfqueue<buffer_t> q;

void recyle() {
    int i = 1;
    while (i <= ITERATIONS) {
        buffer_t b;
        while (!q.poll(b))
            std::this_thread::yield();

        assert(std::stoi(std::string(b.b, b.b + b.size)) == b.data);

        rbuf.release(b.alloc_size);
        ++i;
    }
}

int main(int argc, char const *argv[]) {
    char *p1;
    char *p2;
    char *p3;
    assert(512 == rbuf.acquire(512, p1));
    assert(p1);

    assert(256 == rbuf.acquire(256, p2));
    assert(p2);

    assert(p2 - p1 == 512);

    assert(0 == rbuf.acquire(512, p1));

    assert(rbuf.freespace() == 256);

    assert(0 == rbuf.acquire(512, p3));

    rbuf.release(512);

    assert(768 == rbuf.acquire(512, p3));

    printf("rbuf.freespace(): %lu\n", rbuf.freespace());
    assert(rbuf.freespace() == 0);

    rbuf.release(256);
    printf("rbuf.freespace(): %lu\n", rbuf.freespace());
    assert(256 == rbuf.freespace());
    rbuf.release(768);
    printf("rbuf.freespace(): %lu\n", rbuf.freespace());
    assert(1024 == rbuf.freespace());

    std::thread recyler(recyle);

    int i = 1;
    while (i <= ITERATIONS) {
        std::string s(std::to_string(i));

        buffer_t b;
        char *ptr;
        int size;
        while (!(size = rbuf.acquire(s.size(), ptr)))
            std::this_thread::yield();

        b.b = ptr;
        memcpy(b.b, s.c_str(), s.size());
        b.size = s.size();
        b.alloc_size = size;
        b.data = i;

        while (!q.offer(b))
            std::this_thread::yield();
        ++i;
    }

    recyler.join();
    printf("passed\n");
    return 0;
}
