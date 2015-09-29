#ifndef IRIS_BUFFER_H_
#define IRIS_BUFFER_H_
#include <string.h>
#include <memory>
namespace iris {

struct buffer {
private:
    buffer(const buffer & rhs)=delete;
    struct buffer & operator=(const buffer &rhs)=delete;
public:
    buffer(size_t cap = 0);

    ~buffer();

    void expand(size_t n);

    inline void reset() {
        pos = 0;
    }

    inline size_t freespace() {
        return capacity - pos;
    }

    inline char * write_pointer() {
        return buf + pos;
    }

    inline size_t size() {
        return pos;
    }

    inline operator char*() {
        return buf;
    }
    char       *buf;
    int         pos;
    int         capacity;
};

}
#endif
