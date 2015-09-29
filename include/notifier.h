#ifndef IRIS_NOTIFIER_H_
#define IRIS_NOTIFIER_H_
#include <vector>

namespace iris {

// Bitmask at the lowest bits of a long type
// representing types of notification.
enum ntf_type{
    ntf_msg = 1,           // first bit for message
    ntf_queue_deletion = 2, // second bit for queue deletion
    ntf_type_mask = 3
};

typedef long ntf_t;

class notifier {
public:
    notifier();
    ~notifier();
    // send a notification to the receiver
    void notify(const ntf_t & ntf);
    // wait for notifications
    // return false if timed out or on error
    bool wait(long timeout, std::vector<ntf_t> & ntfs);
    static inline ntf_t to_ntf_t(long data, ntf_type type) {
        return data | (int)type;
    }
    static inline long to_data_t(ntf_t ntf) {
        return ntf & ~ntf_type_mask;
    }
    static inline ntf_type to_ntf_type(ntf_t ntf) {
        return static_cast<ntf_type>(ntf & ntf_type_mask);
    }
private:
    int  m_pipe[2];
    long m_poll_time;
};

}
#endif