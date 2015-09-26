#ifndef IRIS_DEFINE_H_
#define IRIS_DEFINE_H_
#include <sys/uio.h>

#include "buffer.h"

namespace iris {

#define iris_likely(x) __builtin_expect(!!(long)(x), 1)
#define iris_unlikely(x) __builtin_expect(!!(long)(x), 0)

enum level {
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

struct loglet_t {
    const char *                            fmt;
    // @pbuffer holds formatter function address and arguments
    buffer                                  buf;
    loglet_t(const char * format_str, size_t buffer_size = 0):fmt(format_str), buf(buffer_size){}
};

#if defined(IOV_MAX) /* Linux x86 (glibc-2.3.6-3) */
    #define MAX_IOVECS IOV_MAX
#elif defined(MAX_IOVEC) /* Linux ia64 (glibc-2.3.3-98.28) */
    #define MAX_IOVECS MAX_IOVEC
#elif defined(UIO_MAXIOV) /* Linux x86 (glibc-2.2.5-233) */
    #define MAX_IOVECS UIO_MAXIOV
#elif (defined(__FreeBSD__) && __FreeBSD_version < 500000) || defined(__DragonFly__) || defined(__APPLE__) 
    /* - FreeBSD 4.x
     * - MacOS X 10.3.x
     *   (covered in -DKERNEL)
     *  */
    #define MAX_IOVECS 1024
#else
    #error "can't deduce the maximum number of iovec in a readv/writev syscall"
#endif
}
#endif
