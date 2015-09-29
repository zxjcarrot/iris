#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include <define.h>
#include <stream_writer.h>
using namespace iris;

stream_writer::stream_writer(FILE * stream): m_stream(stream), m_fd(-1) {
    if (stream)
        m_fd = fileno(m_stream);
}

void stream_writer::write(const char * msg) {
    size_t len = strlen(msg);
    this->write(msg, len);
}

void stream_writer::write(const char * buffer, size_t len){
    size_t written = 0;
    while (written < len) {
        size_t n = fwrite(buffer + written, 1, len - written, m_stream);
        written += n;
        if (n < len) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "[iris] error, should write %lu byts, only %lu bytes written, reason: %s.\n", len, n, strerror(errno));
            break;
        }
    }
    fflush(m_stream);
}

void stream_writer::write(const std::vector<struct iovec> & vecs) {
    ssize_t n;
    size_t batch_size = std::min((size_t)MAX_IOVECS, vecs.size()), idx = 0;
redo:
    for (; idx < vecs.size(); idx += batch_size) {
        n = writev(m_fd, &vecs[idx], batch_size);
        if (iris_unlikely(n < 0)) {
            if (errno == EINTR)
                goto redo;
            fprintf(stderr, "[iris] error m_fd: %d, vecs.size(): %lu, %ld bytes written, reason: %s.\n", m_fd, vecs.size(), n, strerror(errno));
        }
    }

    fflush(m_stream);
}
