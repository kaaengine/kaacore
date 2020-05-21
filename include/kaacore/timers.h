#pragma once

#include <functional>

#include <SDL.h>

namespace kaacore {

typedef std::uintptr_t TimerID;
typedef std::function<void()> TimerCallback;

extern uint32_t KAACORE_Timer;
void
resolve_timer(TimerID timer_id);
void
destroy_timers();

class Timer {

  public:
    Timer() = default;
    Timer(
        const uint32_t interval, const TimerCallback callback,
        const bool single_shot = true);
    ~Timer();

    void start();
    bool is_running();
    void stop();

  private:
    bool _single_shot;
    TimerID _timer_id;
    uint32_t _interval;
    TimerCallback _callback;
    static inline TimerID _last_timer_id = 0;

    void _start();
    void _stop();
};

}
