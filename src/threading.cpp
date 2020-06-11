#include <mutex>

#include "kaacore/threading.h"

namespace kaacore {

void
SyncedSyscallQueue::finalize_calls()
{
    std::lock_guard lock{this->_queue_mutex};
    for (const auto& func : this->_queued_functions) {
        func();
    }
    this->_queued_functions.clear();
}

} // namespace kaacore
