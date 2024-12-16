#pragma once

#include <array>
#include <functional>
#include <limits>
#include <optional>
#include <random>

namespace kaacore {

template<typename T>
inline constexpr bool always_false_v = false;

template<class T, class M>
static inline constexpr ptrdiff_t
offset_of(const M T::*member)
{
    return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<T*>(0)->*member));
}

template<class T, class M>
static inline constexpr T*
container_of(const M* ptr, const M T::*member)
{
    return reinterpret_cast<T*>(
        reinterpret_cast<intptr_t>(ptr) - offset_of(member)
    );
}

std::default_random_engine&
get_random_engine();

template<typename T>
T
random_uid()
{
    static std::uniform_int_distribution<T> distribution{
        std::numeric_limits<T>::min() + 1, std::numeric_limits<T>::max()
    };
    return distribution(get_random_engine());
}

template<typename T, typename... Args>
size_t
hash_combined_seeded(size_t seed, const T& val, Args&&... args)
{
    seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    if constexpr (sizeof...(args) > 0) {
        seed = hash_combined_seeded(seed, std::forward<Args>(args)...);
    }
    return seed;
}

template<typename T, typename... Args>
size_t
hash_combined(const T& val, Args&&... args)
{
    return hash_combined_seeded(0, std::forward<Args>(args)...);
}

template<typename T, typename Iter>
size_t
hash_iterable(const Iter it_start, const Iter it_end)
{
    size_t seed = 0;
    for (auto it = it_start; it != it_end; ++it) {
        seed ^= std::hash<T>{}(*it) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

template<typename T, size_t N>
inline constexpr std::optional<size_t>
find_array_element(const std::array<T, N>& array, const T& value)
{
    for (size_t index = 0; index < array.size(); index++) {
        if (array[index] == value) {
            return index;
        }
    }
    return std::nullopt;
}

} // namespace kaacore
