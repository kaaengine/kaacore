#include <unordered_map>

#include "kaacore/input.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/timers.h"

namespace kaacore {

struct _TimerData {
    uint32_t interval;
    TimerCallback callback;
    uint32_t next_trigger_time;
    SDL_TimerID internal_timer_id;
};

static TimerID _last_timer_id = 0;
std::unordered_map<TimerID, _TimerData> _timer_data_map;

bool
_map_contains(TimerID timer_id)
{
    auto index = _timer_data_map.find(timer_id);
    return index != _timer_data_map.end();
}

void
_remove_timer(TimerID timer_id)
{
    _timer_data_map.erase(timer_id);
}

static uint32_t
_timer_callback_wrapper(uint32_t interval, void* encoded_id)
{
    SDL_Event event;
    event.type = static_cast<uint32_t>(EventType::_timer_fired);
    event.user.data1 = encoded_id;
    SDL_PushEvent(&event);
    return 0;
}

SDL_TimerID
_spawn_sdl_timer(TimerID timer_id, uint32_t interval)
{
    auto encoded_timer_id = reinterpret_cast<void*>(timer_id);
    auto internal_timer_id =
        SDL_AddTimer(interval, _timer_callback_wrapper, encoded_timer_id);

    if (!internal_timer_id) {
        throw kaacore::exception(SDL_GetError());
    }

    return internal_timer_id;
}

void
resolve_timer(TimerID timer_id)
{
    if (!_map_contains(timer_id)) {
        return;
    }

    _TimerData& timer_data = _timer_data_map[timer_id];
    timer_data.callback();

    if (timer_data.next_trigger_time == 0) {
        return;
    }

    // compensate for time lost due to events processing delay
    auto now = SDL_GetTicks();
    int64_t diff = now - timer_data.next_trigger_time;
    auto next_interval = timer_data.interval - diff;
    timer_data.next_trigger_time = now + next_interval;
    timer_data.internal_timer_id = _spawn_sdl_timer(timer_id, next_interval);
}

void
destroy_timers()
{
    _timer_data_map.clear();
}

Timer::Timer(
    const uint32_t interval, const TimerCallback callback,
    const bool single_shot)
    : _single_shot(single_shot), _timer_id(++_last_timer_id),
      _interval(interval), _callback(callback)
{}

void
Timer::_start()
{
    uint32_t next_trigger_time = 0;
    if (!this->_single_shot) {
        next_trigger_time = SDL_GetTicks() + this->_interval;
    }

    _TimerData timer_data(
        {this->_interval, this->_callback, next_trigger_time, 0});

    timer_data.internal_timer_id =
        _spawn_sdl_timer(this->_timer_id, this->_interval);
    _timer_data_map[this->_timer_id] = timer_data;
}

void
Timer::start()
{
    if (this->_is_running()) {
        this->_stop();
    }

    this->_start();
}

bool
Timer::_is_running()
{
    return _map_contains(this->_timer_id);
}

bool
Timer::is_running()
{
    return this->_is_running();
}

void
Timer::_stop()
{
    SDL_RemoveTimer(_timer_data_map[this->_timer_id].internal_timer_id);
    _remove_timer(this->_timer_id);
}

void
Timer::stop()
{
    if (!this->_is_running()) {
        return;
    }
    this->_stop();
}

}
