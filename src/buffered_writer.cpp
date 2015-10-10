#include <buffered_writer.h>

using namespace iris;

buffered_writer::buffered_writer(writer & wter, size_t cap): m_w(wter), m_buf(nullptr), m_pos(0), m_capacity(cap) {
    if (m_capacity) {
        m_buf = new char[m_capacity];
    }
}

buffered_writer::~buffered_writer() {
    if (m_buf) {
        delete[] m_buf;
        m_buf = nullptr;
        m_pos = m_capacity = 0;
    }
}

void buffered_writer::expand(size_t n) {
    if (n <= m_capacity) 
        return;
    char *tmp_buf = new char[n];
    memcpy(tmp_buf, this->m_buf, this->size());
    delete[] this->m_buf;
    this->m_buf = tmp_buf;
    this->m_capacity = n;
}