#pragma once

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "kaacore/utils.h"

namespace kaacore {

typedef uint32_t UnicodeCodepoint;

typedef std::variant<
    std::basic_string_view<char>, std::basic_string_view<char16_t>,
    std::basic_string_view<char32_t>>
    UnicodeStringViewVariant;

enum class UnicodeRepresentationSize : uint8_t {
    ucs1 = 1,
    ucs2 = 2,
    ucs4 = 4,
};

class UnicodeView {
    const std::byte* _data;
    std::size_t _length;
    UnicodeRepresentationSize _representation_size;

  public:
    class iterator {
        const UnicodeView* _view;
        std::size_t _index;

      public:
        iterator(const UnicodeView* view, const std::size_t index);

        UnicodeCodepoint operator*() const;
        iterator& operator++();
        bool operator==(const iterator& other) const;
        bool operator!=(const iterator& other) const;
    };

    UnicodeView();
    UnicodeView(
        const std::byte* data, const std::size_t length,
        const UnicodeRepresentationSize representation_size
    );
    UnicodeView(
        const std::uint8_t* data, const std::size_t length,
        const UnicodeRepresentationSize representation_size
    );
    UnicodeView(const UnicodeStringViewVariant& view_variant);

    bool operator==(const UnicodeView& other) const;

    iterator begin() const;
    iterator end() const;
    UnicodeStringViewVariant string_view_variant() const;

    UnicodeRepresentationSize representation_size() const;
    std::size_t length() const;
    const std::byte* data() const;

    friend std::hash<UnicodeView>;
};

class UnicodeBuffer {
    std::vector<std::byte> _data_container;
    std::size_t _length;
    UnicodeRepresentationSize _representation_size;

  public:
    UnicodeBuffer();
    UnicodeBuffer(
        const std::byte* data, const std::size_t length,
        const UnicodeRepresentationSize representation_size
    );
    UnicodeBuffer(const UnicodeStringViewVariant& view_variant);
    UnicodeBuffer(const UnicodeView& unicode_view);

    UnicodeView view() const;
};

} // namespace kaacore

namespace std {
using kaacore::hash_iterable;
using kaacore::UnicodeCodepoint;
using kaacore::UnicodeView;

template<>
struct hash<UnicodeView> {
    size_t operator()(const UnicodeView& view) const
    {
        return hash_iterable<UnicodeCodepoint, UnicodeView::iterator>(
            view.begin(), view.end()
        );
    }
};

} // namespace std
