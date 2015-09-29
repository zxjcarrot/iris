#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

#include <string>
#include <system_error>

#include <notifier.h>

using namespace iris;

notifier::notifier() {
    if (::pipe(m_pipe)) {
        std::string error_str = "failed to call pipe(), reason:";
        error_str += strerror(errno);
        throw std::system_error(std::error_code(errno, std::system_category()), error_str);
    }   
    if (fcntl(m_pipe[1], F_SETFL, O_NONBLOCK)) {
        std::string error_str = "failed to set piep's write side to nonblock mode, reason:";
        error_str += strerror(errno);
        throw std::system_error(std::error_code(errno, std::system_category()), error_str);
    }
}

notifier::~notifier() {
    close(m_pipe[0]);
    close(m_pipe[1]);
}

void notifier::notify(const ntf_t & ntf) {
    size_t written = 0;
    while (written < sizeof(ntf_t)) {
        int n = write(m_pipe[1], (char *)&ntf, sizeof(ntf_t) - written);
        if (n <= 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        written += n;
    }
}

bool notifier::wait(long timeout, std::vector<ntf_t> & ntfs) {
    struct pollfd pfd;
    pfd.fd = m_pipe[0];
    pfd.events = POLLIN;

    if(poll(&pfd, 1, timeout) == 1) {
        int flags = fcntl(m_pipe[0], F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(m_pipe[0], F_SETFL, flags);
        while (true) {
            ntf_t ntf;
            size_t readn = 0;
            while (readn < sizeof(ntf_t)) {
                int n = read(m_pipe[0], &ntf, sizeof(ntf_t) - readn);
                if (n <= 0) {
                    if (errno == EINTR)
                        continue;
                    break;
                }
                readn += n;
            }
            if (readn >= sizeof(ntf_t)) {
                ntfs.push_back(ntf);
            } else {
                break;
            }
        }
        flags &= ~O_NONBLOCK;
        fcntl(m_pipe[0], F_SETFL, flags);

        return true;
    }

    return false;
}