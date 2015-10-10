#ifndef IRIS_UTILS_H_
#define IRIS_UTILS_H_
#include <sys/time.h>

#include <cstddef>
namespace iris{

long long get_current_time_in_us();


int round_up_to_next_multiple_of_2(int n);

/* utilitis for making sequence for tuple element retrieval */
template<size_t ...> struct seq {};
template<size_t idx, std::size_t N, std::size_t... S> struct seq_helper: seq_helper<idx + 1, N, S..., idx> {};
template<size_t N, size_t ...S> struct seq_helper<N, N, S...> {
    typedef seq<S...> type;
};
template<size_t N>
struct make_sequence {
    typedef typename seq_helper<0, N>::type type;
};


}
#endif