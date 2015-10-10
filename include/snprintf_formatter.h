#ifndef IRIS_SNPRINTF_FORMATTER_H_
#define IRIS_SNPRINTF_FORMATTER_H_

#include <cstdio>
#include <utility>

#include <assert.h>

#include "formatter.h"
#include "utils.h"
#include "define.h"
#include "buffered_writer.h"

namespace iris {
class snprintf_formatter {
public:

template<typename... Args>
static void format(buffered_writer * bw, const char * fmt, Args&&... args) {
    size_t n;
    while (true) {
        n = std::snprintf(bw->write_pointer(), bw->freespace(), fmt, args...);
        if (iris_unlikely(n < 0)) {
            return;//skip over this formatting if error occurred
        }
        if (iris_unlikely(n >= bw->freespace())) {
            bw->flush(); // flush buffer and retried
            continue;
        }
        bw->inc_write_pointer(n);
        assert(*bw->write_pointer() == 0);
        *bw->write_pointer() = '\n';// replace '\0' with '\n'
        bw->inc_write_pointer(1);
        break; // TODO what to do with failed formatting?
    }

}

    
};
}
#endif