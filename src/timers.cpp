#include <memory>
#include <unordered_map>

#include "kaacore/log.h"
#include "kaacore/timers.h"
#include "kaacore/exceptions.h"

namespace kaacore {

uint32_t KAACORE_Timer = SDL_RegisterEvents(1);

struct _TimerData {
    TimerID timer_id;
    bool single_shot;
    TimerCallback callback;
    SDL_TimerID internal_timer_id;
};

static TimerID _last_timer_id = 0;
std::unordered_map<TimerID, _TimerData> _timer_data_map;

void _remove_timer(TimerID timer_id) {
    _timer_data_map.erase(timer_id);
}

void resolve_timer(TimerID timer_id) {
    _TimerData& timer_data = _timer_data_map[timer_id];
    timer_data.callback();

    if (timer_data.single_shot) {
        _remove_timer(timer_id);
    }
}

static uint32_t _timer_callback_wrapper(uint32_t interval, void* data) {
    auto timer_data_ptr = static_cast<_TimerData*>(data);
    SDL_Event event;
    SDL_memset(&event, 0, sizeof(event));
    event.type = KAACORE_Timer;
    event.user.data1 = &timer_data_ptr->timer_id;
    SDL_PushEvent(&event);

    if (timer_data_ptr->single_shot) {
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

void Timer::start() {
    if (this->is_running()) {
        this->stop();
    }

    _TimerData timer_data(
        {this->_timer_id, this->_single_shot, this->_callback, 0}
    );
    _timer_data_map[this->_timer_id] = timer_data;
    auto timer_data_ptr = &_timer_data_map[this->_timer_id];
    auto internal_timer_id = SDL_AddTimer(
        this->_interval, _timer_callback_wrapper, timer_data_ptr
    );

    if (!internal_timer_id) {
        _remove_timer(this->_timer_id);
        throw kaacore::exception(SDL_GetError());
    }

    timer_data_ptr->internal_timer_id = internal_timer_id;
}

bool Timer::is_running() {
    auto index = _timer_data_map.find(this->_timer_id);
    return index != _timer_data_map.end();
}

void Timer::stop() {
    if (!this->is_running()) {
        return;
    }

    SDL_RemoveTimer(_timer_data_map[this->_timer_id].internal_timer_id);
    _remove_timer(this->_timer_id);
}

}
