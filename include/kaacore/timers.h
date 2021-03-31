#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "kaacore/clock.h"

namespace kaacore {

class Scene;
class TimersManager;

struct TimerContext {
    Duration interval;
    Scene* scene;
};

using TimerID = uint32_t;
using TimerCallback = std::function<Duration(TimerContext context)>;

struct _TimerState {
    _TimerState(TimerID id, TimerCallback&& callback);

    TimerID id;
    TimerCallback callback;
    std::atomic<bool> is_running;
};

class Timer {
  public:
    Timer() = default;
    Timer(TimerCallback callback);

    void start_global(const Duration interval);
    void start(const Duration interval, Scene* const scene);
    bool is_running() const;
    void stop();

  private:
    std::shared_ptr<_TimerState> _state;

    void _start(const Duration interval, TimersManager& manager);

    friend class TimersManager;
};

class TimersManager {
  public:
    TimersManager();
    TimersManager(Scene* const scene);
    void start(const Duration interval, Timer& timer);
    void process(const HighPrecisionDuration dt);
    TimePoint time_point() const;

  private:
    using _AwaitingState =
        std::tuple<TimerID, Duration, std::weak_ptr<_TimerState>>;

    struct _InvocationInstance {
        _InvocationInstance(
            TimerID invocation_id, Duration interval, TimePoint triggered_at,
            std::weak_ptr<_TimerState>&& state);

        TimerID invocation_id;
        Duration interval;
        TimePoint triggered_at;
        std::weak_ptr<_TimerState> state;

        inline TimePoint fire_at() const
        {
            return this->triggered_at +
                   std::chrono::duration_cast<HighPrecisionDuration>(
                       this->interval);
        }
    };

    std::mutex _lock;
    Scene* const _scene;
    HighPrecisionDuration _dt_accumulator;
    struct {
        bool is_dirty = false;
        std::vector<_InvocationInstance> data;
    } _queue;
    struct {
        std::atomic<bool> is_dirty = false;
        std::vector<_AwaitingState> data;
    } _awaiting_timers;

    // FIXME not thread safe
    static inline TimerID _last_timer_id = 0;
};

}
