#pragma once

#include <cstddef>

namespace kaacore {

class Memory {
  public:
    Memory();
    Memory(Memory&& other);
    ~Memory();
    Memory(const Memory& other);
    Memory& operator=(const Memory& other);
    Memory& operator=(Memory&& other);
    static Memory copy(const std::byte* memory, std::size_t size);
    static Memory reference(const std::byte* memory, std::size_t size);

    void destroy();
    const std::byte* get() const;
    const std::size_t size() const;
    const std::byte* release();

  private:
    const std::byte* _memory;
    std::size_t _size;
    bool _owning;

    Memory(const std::byte* memory, const std::size_t size, const bool owning);

    void _copy_or_referene(const std::byte* memory);
};
} // namespace kaacore
