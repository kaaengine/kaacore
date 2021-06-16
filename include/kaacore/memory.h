#pragma once

#include <cstddef>
#include <memory>

namespace kaacore {

class Memory {
  public:
    Memory();
    Memory(Memory&& other);
    Memory(const Memory& other);
    Memory& operator=(const Memory& other);
    Memory& operator=(Memory&& other);
    operator bool() const;
    bool operator==(const Memory& other);
    static Memory copy(const std::byte* memory, std::size_t size);
    static Memory reference(const std::byte* memory, std::size_t size);

    void destroy();
    const std::byte* get() const;
    const std::size_t size() const;

  private:
    std::size_t _size;
    const std::byte* _reference;
    std::shared_ptr<std::byte> _owned_memory;

    Memory(const std::byte* memory, const std::size_t size);
    Memory(std::shared_ptr<std::byte>&& memory, const std::size_t size);
};

} // namespace kaacore

namespace std {
template<>
struct hash<kaacore::Memory> {
    size_t operator()(const kaacore::Memory& memory) const
    {
        return std::hash<const std::byte*>{}(memory.get());
    }
};
}
