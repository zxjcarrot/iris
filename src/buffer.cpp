#include <buffer.h>

using namespace iris;

buffer::buffer(size_t cap):buf(nullptr), pos(0), capacity(cap){
    if (capacity) {
        buf = new char[capacity];
    }
}

buffer::~buffer() {
    if (buf) {
        delete[] buf;
        buf = nullptr;
        pos = capacity = 0;
    }
}

void buffer::expand(size_t n) {
    if (n <= capacity) 
        return;
    char *tmp_buf = new char[n];
    memcpy(tmp_buf, this->buf, this->size());
    delete[] this->buf;
    this->buf = tmp_buf;
    this->capacity = n;
}
