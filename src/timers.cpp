#include <algorithm>
#include <unordered_map>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"
#include "kaacore/scenes.h"
#include "kaacore/timers.h"

namespace kaacore {

_TimerState::_TimerState(TimerId id, TimerCallback&& callback)
    : id(id), callback(std::move(callback))
{}

Timer::Timer(TimerCallback callback)
{
    this->_state = std::make_shared<_TimerState>(0, std::move(callback));
}

void
Timer::start(const Duration interval, Scene* const scene)
{
    this->_start(interval, scene->timers);
}

void
Timer::start_global(const Duration interval)
{
    get_engine()->timers.start(interval, *this);
}

void
Timer::_start(const Duration interval, TimersManager& manager)
{
    KAACORE_CHECK(interval > 0.s, "Timer interval must be greater than zero.");
    KAACORE_CHECK(
        interval.count() != INFINITY, "Timer interval cannot be infinity."
    );
    if (this->is_running()) {
        this->stop();
    }
    manager.start(interval, *this);
}

bool
Timer::is_running() const
{
    return this->_state->is_running.load(std::memory_order_acquire);
}

void
Timer::stop()
{
    this->_state->is_running.store(false, std::memory_order_release);
}

TimersManager::TimersManager() : _scene(nullptr) {}

TimersManager::TimersManager(Scene* const scene) : _scene(scene) {}

TimersManager::_InvocationInstance::_InvocationInstance(
    TimerId invocation_id, Duration interval, TimePoint triggered_at,
    std::weak_ptr<_TimerState>&& state
)
    : invocation_id(invocation_id), interval(interval),
      triggered_at(triggered_at), state(state)
{}

void
TimersManager::start(const Duration interval, Timer& timer)
{
    {
        std::unique_lock<std::mutex> lock{this->_lock};
        auto state = timer._state;
        auto invocation_id =
            this->_last_id.fetch_add(1, std::memory_order_relaxed);
        state->id = invocation_id;
        state->is_running.store(true, std::memory_order_release);
        this->_awaiting_timers.data.emplace_back(
            invocation_id, interval, state
        );
    }
    this->_awaiting_timers.is_dirty.store(true, std::memory_order_release);
}

void
TimersManager::process(const HighPrecisionDuration dt)
{
    if (this->_awaiting_timers.is_dirty.load(std::memory_order_acquire)) {
        auto now = this->time_point();
        {
            std::unique_lock<std::mutex> lock{this->_lock};
            for (auto& awaiting_state : this->_awaiting_timers.data) {
                auto& [invocation_id, interval, weak_state] = awaiting_state;
                this->_queue.data.emplace_back(
                    invocation_id, interval, now, std::move(weak_state)
                );
            }
            this->_awaiting_timers.data.clear();
        }
        this->_queue.is_dirty = true;
        this->_awaiting_timers.is_dirty.store(false, std::memory_order_release);
    }

    this->_dt_accumulator += dt;

    if (this->_queue.is_dirty) {
        std::sort(
            this->_queue.data.begin(), this->_queue.data.end(),
            [](const auto& lhs, const auto& rhs) -> bool {
                return lhs.fire_at() > rhs.fire_at();
            }
        );
        this->_queue.is_dirty = false;
    }

    auto now = this->time_point();
    for (auto it = this->_queue.data.rbegin(); it != this->_queue.data.rend();
         ++it) {
        if (it->fire_at() > now) {
            break;
        }

        auto invocation_id = it->invocation_id;
        auto state = it->state.lock();
        if (not state) {
            // timer deleted
            this->_queue.data.pop_back();
            continue;
        }

        if (not state->is_running.load(std::memory_order_acquire) or
            state->id != invocation_id) {
            // timer outdated
            this->_queue.data.pop_back();
            continue;
        }

        struct TimerContext context = {it->interval, this->_scene};
        auto next_interval = state->callback(context);
        if (not(next_interval > 0.s)) {
            this->_queue.data.pop_back();
            state->is_running.store(false, std::memory_order_release);
            continue;
        }

        it->triggered_at = now;
        it->interval = next_interval;
        this->_queue.is_dirty = true;
    }
}

TimePoint
TimersManager::time_point() const
{
    return TimePoint(this->_dt_accumulator);
}

} // namespace kaacore
