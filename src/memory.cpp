#include <algorithm>

#include "kaacore/log.h"
#include "kaacore/memory.h"

namespace kaacore {

Memory::Memory() : _memory(nullptr), _size(0), _owning(false) {}

Memory::Memory(
    const std::byte* memory, const std::size_t size, const bool owning)
    : _size(size), _owning(owning)
{
    this->_copy_or_referene(memory);
}

Memory::~Memory()
{
    this->destroy();
}

Memory::Memory(const Memory& other) : _size(other._size), _owning(other._owning)
{
    KAACORE_LOG_DEBUG("Copying Memory: {}.", fmt::ptr(this->_memory));
    this->_copy_or_referene(other._memory);
}

Memory::Memory(Memory&& other)
{
    KAACORE_LOG_DEBUG("Moving Memory: {}.", fmt::ptr(this->_memory));
    this->_size = other._size;
    this->_memory = other._memory;
    this->_owning = other._owning;

    other._size = 0;
    other._memory = nullptr;
    other._owning = false;
}

Memory&
Memory::operator=(const Memory& other)
{
    if (this == &other) {
        return *this;
    }

    KAACORE_LOG_DEBUG("Copying Memory: {}.", fmt::ptr(this->_memory));
    if (this->_owning) {
        this->destroy();
    }

    this->_size = other._size;
    this->_owning = other._owning;
    this->_copy_or_referene(other._memory);
    return *this;
}

Memory&
Memory::operator=(Memory&& other)
{
    if (this == &other) {
        return *this;
    }

    KAACORE_LOG_DEBUG("Moving Memory: {}.", fmt::ptr(this->_memory));
    if (this->_owning) {
        this->destroy();
    }

    this->_size = other._size;
    this->_memory = other._memory;
    this->_owning = other._owning;

    other._size = 0;
    other._memory = nullptr;
    other._owning = false;

    return *this;
}

Memory
Memory::copy(const std::byte* memory, std::size_t size)
{
    return Memory(memory, size, true);
}

Memory
Memory::reference(const std::byte* memory, std::size_t size)
{
    return Memory(memory, size, false);
}

void
Memory::destroy()
{
    if (this->_memory and this->_owning) {
        delete[] this->_memory;
    }
}

const std::byte*
Memory::get() const
{
    return this->_memory;
}

const std::size_t
Memory::size() const
{
    return this->_size;
}

const std::byte*
Memory::release()
{
    this->_owning = false;
    return this->get();
}

void
Memory::_copy_or_referene(const std::byte* memory)
{
    if (this->_owning) {
        auto destination_memory = new std::byte[this->_size];
        std::memcpy(destination_memory, memory, this->_size);
        this->_memory = destination_memory;
    } else {
        this->_memory = memory;
    }
}

} // namespace kaacore
