#include <condition_variable>
#include <initializer_list>
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

template<typename T>
class AwaitableStateEnum {
  public:
    AwaitableStateEnum(T state) : _current_state(state) {}

    T retrieve()
    {
        std::lock_guard lock{this->_mutex};
        return this->_current_state;
    }

    void set(T new_state)
    {
        std::lock_guard lock{this->_mutex};
        this->_current_state = new_state;
        this->_condition_var.notify_one();
    }

    void wait(T expected_state)
    {
        std::unique_lock lock{this->_mutex};
        this->_condition_var.wait(lock, [this, expected_state] {
            return this->_current_state == expected_state;
        });
    }

    T wait(std::initializer_list<T> expected_states)
    {
        std::unique_lock lock{this->_mutex};
        this->_condition_var.wait(lock, [this, expected_states] {
            for (const auto& state : expected_states) {
                if (this->_current_state == state) {
                    return true;
                }
            }
            return false;
        });
        return this->_current_state;
    }

  private:
    T _current_state;
    std::condition_variable _condition_var;
    std::mutex _mutex;
};

} // namespace kaacore
