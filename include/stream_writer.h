#ifndef IRIS_STREAM_WRITER_H_
#define IRIS_STREAM_WRITER_H_


#include <cstdio>
#include <string>

#include "writer.h"

namespace iris {

// A log writer that appends logs to a standard stream.
class stream_writer: public writer {
public:
    stream_writer(FILE * stream);
    virtual void write(const char * msg);
    virtual void write(const struct iovec & vec);
    virtual void write(const std::vector<struct iovec> & logs);
protected:
    FILE *      m_stream;
    int         m_fd;
};

}

#endif