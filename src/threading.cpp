#include <mutex>

#include "kaacore/threading.h"

namespace kaacore {

void
DelayedSyscallQueue::enqueue_function(DelayedSyscallFunction&& func)
{
    std::lock_guard lock{this->_queue_mutex};
    this->_delayed_functions.emplace_back(std::move(func));
}

void
DelayedSyscallQueue::call_all()
{
    std::lock_guard lock{this->_queue_mutex};
    for (const auto& func : this->_delayed_functions) {
        func();
    }
    this->_delayed_functions.clear();
}

} // namespace kaacore
