#ifndef IRIS_FILE_WRITER_H_
#define IRIS_FILE_WRITER_H_


#include <cstdio>
#include <string>

#include "stream_writer.h"

namespace iris {

// A log writer that appends logs to a file.
class file_writer: public stream_writer {
public:
    file_writer(const char * filename);
    ~file_writer();
private:
    std::string m_filename;
};

}

#endif