#include <numeric>

#include "kaacore/clock.h"

namespace kaacore {

_DurationRingBuffer::_DurationRingBuffer(size_t size) : _size(size), _cursor(0)
{
    this->_data.resize(size, 0us);
}

void
_DurationRingBuffer::reset()
{
    this->_data.clear();
}

void
_DurationRingBuffer::push(const Microseconds duration)
{
    this->_cursor %= this->_size;
    this->_data[this->_cursor++] = duration;
}

Microseconds
_DurationRingBuffer::average() const
{
    auto sum = std::accumulate(this->_data.begin(), this->_data.end(), 0us);
    return sum / this->_data.size();
}

Clock::Clock()
{
    this->touch();
}

Microseconds
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

Microseconds
Clock::average_duration() const
{
    return this->_buffer.average();
}

TimePoint
Clock::now()
{
    return std::chrono::time_point_cast<Microseconds>(DefaultClock::now());
}

}
