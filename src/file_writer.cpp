#include <file_writer.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


using namespace iris;

file_writer::file_writer(const char * filename): stream_writer(nullptr) {
    FILE *fp = fopen(filename, "a");
    if (fp == nullptr) {
        std::string error_string = "[iris] failed to open log file <";
        error_string += filename;
        error_string += ">, reason: ";
        error_string += strerror(errno);
        throw error_string;
    }
    m_stream = fp;
    m_fd = fileno(m_stream);
}

file_writer::~file_writer() {
    if (m_stream == nullptr)
        return;
    fclose(m_stream);
    m_stream = nullptr;
    m_fd = -1;
}

void file_writer::write(const char * buffer, size_t len) {
    size_t written = 0;
    while (written < len) {
        ssize_t n = ::write(m_fd, buffer + written, len - written);
        if (n <= 0) {
            if (errno == EINTR)
                continue;
            fprintf(stderr, "[iris] error, should write %lu byts, only %lu bytes written, reason: %s.\n", len, n, strerror(errno));
            break;
        }
        written += n;
    }
}