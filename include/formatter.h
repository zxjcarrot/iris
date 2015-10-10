#ifndef IRIS_FORMATTER_H_
#define IRIS_FORMATTER_H_
#include "define.h"
#include "buffered_writer.h"
#include "utils.h"

namespace iris {
// fomatter do the real formatting logic, output into @obuf
// return false if there is not enough space in @obuf.
typedef void (*formatter_type)(const loglet_t & l, buffered_writer & w);

template<typename Formatter, typename...Args, std::size_t...Indexes>
static void call_format(buffered_writer & w, const std::tuple<Args...> & args, seq<Indexes...>) {
    return Formatter::format(&w, std::move(std::get<Indexes>(args))...);
}

template<typename Formatter, typename... Args>
static void formatter_caller(const loglet_t & l, buffered_writer & w) {
    const size_t args_offset = sizeof(formatter_type);

    typedef std::tuple<Args...> Args_t;
    Args_t & args = *reinterpret_cast<Args_t*>(l.rbuf_ptr + args_offset);
    typename make_sequence<sizeof...(Args)>::type indexes;
    call_format<Formatter>(w, args, indexes);
    //deconstruct parameter pack
    args.~Args_t();
}

}
#endif