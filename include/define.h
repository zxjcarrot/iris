#ifndef IRIS_DEFINE_H_
#define IRIS_DEFINE_H_
#include <sys/uio.h>

namespace iris {

#define iris_likely(x) __builtin_expect(!!(long)(x), 1)
#define iris_unlikely(x) __builtin_expect(!!(long)(x), 0)

struct loglet_t {
    // @pbuffer holds formatter function address and arguments
    char                                  * rbuf_ptr;
    size_t                                  rbuf_alloc_size;
    loglet_t(char * ptr = nullptr, size_t alloc_size = 0):rbuf_ptr(ptr), rbuf_alloc_size(alloc_size) {}
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
