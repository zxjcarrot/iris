#ifndef IRIS_SEVERITY_LOGGER_H_
#define IRIS_SEVERITY_LOGGER_H_

#include "snprintf_formatter.h"
#include "base_logger.h"

namespace iris {

enum severity_level {
    UNSET,
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};
extern severity_level thread_severity_level;

// a level logger provides per-thread severity level filtering .
class level_logger: public base_logger {
public:
    level_logger(writer * pwriter = nullptr,
                severity_level default_level = INFO,
                size_t scan_interval = 100,
                size_t output_buffer_size = 102400,
                size_t default_thread_ringbuf_size = 102400,
                size_t default_thread_queue_size = 5000):
        base_logger(pwriter, scan_interval, 
                    output_buffer_size, 
                    default_thread_ringbuf_size,
                    default_thread_queue_size),
        m_default_level(default_level) {}

    // set the severity level for the current thread
    void set_severity_level(severity_level level) {
        thread_severity_level = level;
    }

    template<typename... Args>
    void log(severity_level level, const char * fmt, Args&&... args) {
        if (iris_unlikely(thread_severity_level == UNSET))
            thread_severity_level = m_default_level;
        if (thread_severity_level <= level) {
            base_logger::log<snprintf_formatter>(fmt, std::forward<Args>(args)...);    
        }
    }

    template<typename... Args>
    void trace(const char * fmt, Args&&... args) {
        log(TRACE, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debug(const char * fmt, Args&&... args) {
        log(DEBUG, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void info(const char * fmt, Args&&... args) {
        log(INFO, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warn(const char * fmt, Args&&... args) {
        log(WARN, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void error(const char * fmt, Args&&... args) {
        log(ERROR, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatal(const char * fmt, Args&&... args) {
        log(FATAL, fmt, std::forward<Args>(args)...);
    }
private:
    severity_level m_default_level;
};

}
#endif