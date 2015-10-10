#ifndef IRIS_WRITER_H_
#define IRIS_WRITER_H_

#include <sys/uio.h>

#include <vector>
namespace iris {

// interface for log writting policy
class writer {
public:
    virtual void write(const char * msg) = 0;
    virtual void write(const char * buffer, size_t len) = 0;
};

}

#endif