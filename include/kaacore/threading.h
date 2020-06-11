#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <initializer_list>
#include <mutex>
#include <vector>

namespace kaacore {

class SyncedSyscallQueue {
  public:
    template<typename T>
    T make_sync_call(std::function<T()>&& sync_func)
    {
        std::promise<T> result_promise;
        auto result_future = result_promise.get_future();
        auto call_wrapper = [&result_promise, &sync_func]() {
            // TODO exceptions
            if constexpr (std::is_void_v<T>) {
                sync_func();
                result_promise.set_value();
            } else {
                T ret = sync_func();
                result_promise.set_value(std::move(ret));
            }
        };

        {
            std::lock_guard lock{this->_queue_mutex};
            this->_queued_functions.push_back(std::move(call_wrapper));
        }

        result_future.wait();
        return result_future.get();
    }

    void finalize_calls();

  private:
    std::vector<std::function<void()>> _queued_functions;
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

    template<typename Duration>
    bool wait_for(T expected_state, Duration dur)
    {
        std::unique_lock lock{this->_mutex};
        return this->_condition_var.wait_for(lock, dur, [this, expected_state] {
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
