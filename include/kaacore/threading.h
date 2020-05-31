#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace kaacore {

typedef std::function<void()> DelayedSyscallFunction;

class DelayedSyscallQueue {
  public:
    void enqueue_function(DelayedSyscallFunction&& func);
    void call_all();

  private:
    std::vector<DelayedSyscallFunction> _delayed_functions;
    std::mutex _queue_mutex;
};

} // namespace kaacore
