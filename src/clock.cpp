#include <numeric>

#include "kaacore/clock.h"

namespace kaacore {

DurationRingBuffer::DurationRingBuffer(size_t size) : _size(size), _cursor(0)
{
    this->_data.resize(size, 0us);
}

void
DurationRingBuffer::reset()
{
    this->_data.clear();
}

void
DurationRingBuffer::push(const HighPrecisionDuration duration)
{
    this->_cursor %= this->_size;
    this->_data[this->_cursor++] = duration;
}

HighPrecisionDuration
DurationRingBuffer::average() const
{
    auto sum = std::accumulate(this->_data.begin(), this->_data.end(), 0us);
    return sum / this->_data.size();
}

Clock::Clock()
{
    this->touch();
}

HighPrecisionDuration
Clock::measure()
{
    TimePoint now = this->now();
    auto elapsed = now - this->_last_measurement;
    this->_buffer.push(elapsed);
    this->_last_measurement = now;
    return elapsed;
}

void
Clock::touch()
{
    this->_last_measurement = this->now();
}

void
Clock::reset()
{
    this->touch();
    this->_buffer.reset();
}

HighPrecisionDuration
Clock::average_duration() const
{
    return this->_buffer.average();
}

TimePoint
Clock::now()
{
    return std::chrono::time_point_cast<HighPrecisionDuration>(
        DefaultClock::now()
    );
}

} // namespace kaacore
