#pragma once

#include <functional>

#include <SDL.h>

namespace kaacore {

typedef uint32_t TimerID;
typedef std::function<void()> TimerCallback;

extern uint32_t KAACORE_Timer;
void resolve_timer(TimerID timer_id);

class Timer {

public:
    Timer() = default;
    Timer(const uint32_t interval,
        const TimerCallback callback, const bool single_shot=true
    );

    void start();
    bool is_running();
    void stop();

private:
    bool _single_shot;
    uint32_t _timer_id;
    uint32_t _interval;
    TimerCallback _callback;
};

}
