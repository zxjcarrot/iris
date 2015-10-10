#ifndef IRIS_BUFFERED_WRITER_H_
#define IRIS_BUFFERED_WRITER_H_
#include <string.h>
#include <memory>

#include "writer.h"

namespace iris {

struct buffered_writer {
private:
    buffered_writer(const buffered_writer & rhs)=delete;
    struct buffered_writer & operator=(const buffered_writer &rhs)=delete;
    writer     &m_w;

    char       *m_buf;
    int         m_pos;
    int         m_capacity;
public:
    buffered_writer(writer & w, size_t cap = 0);

    ~buffered_writer();

    void expand(size_t n);

    inline void reset() {
        m_pos = 0;
    }

    inline char * reserve(size_t s) {
        if (freespace() < s) {
            flush();
            if (freespace() < s)
                throw std::bad_alloc();
        }

        return write_pointer();
    }

    inline size_t freespace() {
        return m_capacity - m_pos;
    }

    inline void inc_write_pointer(size_t s) {
        m_pos += s;
    }

    inline char * write_pointer() {
        return m_buf + m_pos;
    }

    inline size_t size() {
        return m_pos;
    }

    inline operator char*() {
        return m_buf;
    }

    inline void flush() {
        m_w.write(m_buf, m_pos);
        reset();
    }
};

}
#endif
