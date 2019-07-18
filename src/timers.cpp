#include <memory>
#include <unordered_map>
#include <mutex>

#include "kaacore/log.h"
#include "kaacore/timers.h"
#include "kaacore/exceptions.h"

namespace kaacore {

uint32_t KAACORE_Timer = SDL_RegisterEvents(1);

struct _TimerData {
    bool single_shot;
    TimerCallback callback;
    SDL_TimerID internal_timer_id;
};

static TimerID _last_timer_id = 0;
std::mutex timer_lock;
std::unordered_map<TimerID, _TimerData> _timer_data_map;

bool _map_contains(TimerID timer_id) {
    auto index = _timer_data_map.find(timer_id);
    return index != _timer_data_map.end();
}

void _remove_timer(TimerID timer_id) {
    _timer_data_map.erase(timer_id);
}

void resolve_timer(TimerID timer_id) {
    TimerCallback callback;
    {
        std::lock_guard<std::mutex> lock(timer_lock);

        if (!_map_contains(timer_id)) {
            return;
        }

        auto timer_data = _timer_data_map[timer_id];
        callback = timer_data.callback;

        if (timer_data.single_shot) {
            _remove_timer(timer_id);
        }
    }
    callback();
}

static uint32_t _timer_callback_wrapper(uint32_t interval, void* encoded_id) {
    std::lock_guard<std::mutex> lock(timer_lock);

    auto timer_id = reinterpret_cast<uintptr_t>(encoded_id);
    if (!_map_contains(timer_id)) {
        return 0;
    }

    SDL_Event event;
    event.type = KAACORE_Timer;
    event.user.data1 = encoded_id;
    SDL_PushEvent(&event);

    auto timer_data = _timer_data_map[timer_id];
    if (timer_data.single_shot) {
        return 0;
    }
    return interval;
}

Timer::Timer(
    const uint32_t interval,
    const TimerCallback callback, const bool single_shot
) :
    _single_shot(single_shot), _timer_id(++_last_timer_id),
    _interval(interval), _callback(callback)
{}

void Timer::_start() {
    _TimerData timer_data({this->_single_shot, this->_callback, 0});
    _timer_data_map[this->_timer_id] = timer_data;

    auto encoded_timer_id = reinterpret_cast<void*>(this->_timer_id);
    auto internal_timer_id = SDL_AddTimer(
        this->_interval, _timer_callback_wrapper, encoded_timer_id
    );

    if (!internal_timer_id) {
        _remove_timer(this->_timer_id);
        throw kaacore::exception(SDL_GetError());
    }

    _timer_data_map[this->_timer_id].internal_timer_id = internal_timer_id;
}

void Timer::start() {
    std::lock_guard<std::mutex> lock(timer_lock);

    if (this->_is_running()) {
        this->_stop();
    }

    this->_start();
}

bool Timer::_is_running() {
    return _map_contains(this->_timer_id);
}

bool Timer::is_running() {
    std::lock_guard<std::mutex> lock(timer_lock);

    return this->_is_running();
}

void Timer::_stop() {
    SDL_RemoveTimer(_timer_data_map[this->_timer_id].internal_timer_id);
    _remove_timer(this->_timer_id);
}

void Timer::stop() {
    std::lock_guard<std::mutex> lock(timer_lock);

    if (!this->_is_running()) {
        return;
    }
    this->_stop();
}

}
