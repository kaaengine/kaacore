#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "kaacore/clock.h"

namespace kaacore {

using TimerID = uint32_t;
using TimerCallback = std::function<Seconds(Seconds interval)>;

struct _TimerState {
    // TODO: CHECK IF PYTHON CALLBACK IS COPIED OR MOVED
    _TimerState(TimerID id, TimerCallback&& callback);

    TimerID id;
    TimerCallback callback;
    std::atomic<bool> is_running;
};

class Scene;
class TimersManager;

class Timer {
  public:
    Timer() = default;
    Timer(TimerCallback callback);

    void start_global(const Seconds interval);
    void start(const Seconds interval, Scene* const scene);
    bool is_running() const;
    void stop();

  private:
    std::shared_ptr<_TimerState> _state;

    void _start(const Seconds interval, TimersManager& manager);

    friend class TimersManager;
};

class TimersManager {
  public:
    void start(const Seconds interval, Timer& timer);
    void process(const Microseconds dt);
    TimePoint time_point() const;

  private:
    using _AwaitingState =
        std::tuple<TimerID, Seconds, std::weak_ptr<_TimerState>>;

    struct _InvocationInstance {
        _InvocationInstance(
            TimerID invocation_id, Seconds interval, TimePoint triggered_at,
            std::weak_ptr<_TimerState>&& state);

        TimerID invocation_id;
        Seconds interval;
        TimePoint triggered_at;
        std::weak_ptr<_TimerState> state;

        inline TimePoint fire_at() const
        {
            return this->triggered_at +
                   std::chrono::duration_cast<Microseconds>(this->interval);
        }
    };

    std::mutex _lock;
    Microseconds _dt_accumulator;
    struct {
        bool is_dirty = false;
        std::vector<_InvocationInstance> data;
    } _queue;
    struct {
        std::atomic<bool> is_dirty = false;
        std::vector<_AwaitingState> data;
    } _awaiting_timers;

    static inline TimerID _last_timer_id = 0;
};

}
