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
    fsync(m_fd);
    fclose(m_stream);
    m_stream = nullptr;
    m_fd = -1;

}
