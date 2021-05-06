#include <algorithm>

#include "kaacore/log.h"
#include "kaacore/memory.h"

namespace kaacore {

Memory::Memory() : _reference(nullptr), _size(0) {}

Memory::Memory(const std::byte* memory, const std::size_t size)
    : _reference(memory), _size(size)
{}

Memory::Memory(std::shared_ptr<std::byte>&& memory, const std::size_t size)
    : _owned_memory(std::move(memory)), _size(size)
{}

Memory::Memory(const Memory& other)
{
    KAACORE_LOG_DEBUG("Copying Memory: {}.", fmt::ptr(other.get()));
    this->_size = other._size;
    this->_reference = other._reference;
    this->_owned_memory = other._owned_memory;
}

Memory::Memory(Memory&& other)
{
    KAACORE_LOG_DEBUG("Moving Memory: {}.", fmt::ptr(other.get()));
    this->_size = other._size;
    this->_reference = other._reference;
    this->_owned_memory = std::move(other._owned_memory);

    other._size = 0;
    other._reference = nullptr;
}

Memory&
Memory::operator=(const Memory& other)
{
    if (this == &other) {
        return *this;
    }

    KAACORE_LOG_DEBUG("Copying Memory: {}.", fmt::ptr(other.get()));
    this->_size = other._size;
    this->_reference = other._reference;
    this->_owned_memory = other._owned_memory;
    return *this;
}

Memory&
Memory::operator=(Memory&& other)
{
    if (this == &other) {
        return *this;
    }

    KAACORE_LOG_DEBUG("Moving Memory: {}.", fmt::ptr(other.get()));
    this->_size = other._size;
    this->_reference = other._reference;
    this->_owned_memory = std::move(other._owned_memory);

    other._size = 0;
    other._reference = nullptr;
    return *this;
}

Memory::operator bool() const
{
    return bool(this->get());
}

bool
Memory::operator==(const Memory& other)
{
    return this->get() == other.get();
}

Memory
Memory::copy(const std::byte* memory, std::size_t size)
{
    auto destination_memory = new std::byte[size];
    std::memcpy(destination_memory, memory, size);
    return Memory(std::shared_ptr<std::byte>(destination_memory), size);
}

Memory
Memory::reference(const std::byte* memory, std::size_t size)
{
    return Memory(memory, size);
}

void
Memory::destroy()
{
    this->_reference = nullptr;
    this->_owned_memory.reset();
    this->_size = 0;
}

const std::byte*
Memory::get() const
{
    if (this->_owned_memory) {
        return this->_owned_memory.get();
    }
    return this->_reference;
}

const std::size_t
Memory::size() const
{
    return this->_size;
}

} // namespace kaacore
