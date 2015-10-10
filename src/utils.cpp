#include <utils.h>

long long iris::get_current_time_in_us() {
    struct timeval v;
    gettimeofday(&v, nullptr);
    return (long long)v.tv_sec * 1000000 + v.tv_usec;
}


int iris::round_up_to_next_multiple_of_2(int n) {
    int bits = 0, ones = 0, oldn = n;
    while(n) {
        if (n & 1)
            ++ones;
        ++bits;
        n >>= 1;
    }

    if (ones == 1) // already a multiple of 2
        return oldn;
    return 1 << bits;
}