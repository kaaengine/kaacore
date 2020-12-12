#pragma once

#include <chrono>
#include <vector>

namespace kaacore {
using namespace std::chrono_literals;

using Seconds = std::chrono::duration<long double>;
using Microseconds = std::chrono::microseconds;
using DefaultClock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<DefaultClock, Microseconds>;

class DurationRingBuffer {
  public:
    DurationRingBuffer(size_t size = 10);
    void reset();
    void push(Microseconds duration);
    Microseconds average() const;

  private:
    uint32_t _size;
    uint32_t _cursor;
    std::vector<Microseconds> _data;
};

class Clock {
  public:
    Clock();
    Microseconds measure();
    Microseconds average_duration() const;
    void touch();
    void reset();
    static TimePoint now();

  private:
    TimePoint _last_measurement;
    DurationRingBuffer _buffer;
};

}
