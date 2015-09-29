#ifndef IRIS_UTILS_H_
#define IRIS_UTILS_H_
#include <sys/time.h>

namespace iris{

inline static long long get_current_time_in_us() {
    struct timeval v;
    gettimeofday(&v, nullptr);
    return (long long)v.tv_sec * 1000000 + v.tv_usec;
}


inline static static int next_multiple_of_2(int n) {
    int bits = 0;
    while(n) {
        ++bits;
        n >>= 1;
    }

    return 1 << bits;
}

}
#endif