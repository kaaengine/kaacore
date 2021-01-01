#pragma once

#include <chrono>
#include <vector>

namespace kaacore {
using namespace std::chrono_literals;

using Duration = std::chrono::duration<long double>;
using HighPrecisionDuration = std::chrono::microseconds;
using DefaultClock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<DefaultClock, HighPrecisionDuration>;

class DurationRingBuffer {
  public:
    DurationRingBuffer(size_t size = 10);
    void reset();
    void push(HighPrecisionDuration duration);
    HighPrecisionDuration average() const;

  private:
    size_t _size;
    uint32_t _cursor;
    std::vector<HighPrecisionDuration> _data;
};

class Clock {
  public:
    Clock();
    HighPrecisionDuration measure();
    HighPrecisionDuration average_duration() const;
    void touch();
    void reset();
    static TimePoint now();

  private:
    TimePoint _last_measurement;
    DurationRingBuffer _buffer;
};

}
