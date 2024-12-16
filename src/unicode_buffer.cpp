#include "kaacore/unicode_buffer.h"

#include "kaacore/exceptions.h"
#include "kaacore/utils.h"

namespace kaacore {

UnicodeView::UnicodeView()
    : _data(nullptr), _length(0),
      _representation_size(UnicodeRepresentationSize::ucs1)
{}

UnicodeView::UnicodeView(
    const std::byte* data, const std::size_t length,
    const UnicodeRepresentationSize representation_size
)
    : _data(const_cast<std::byte*>(data)), _length(length),
      _representation_size(representation_size)
{}

UnicodeView::UnicodeView(
    const std::uint8_t* data, const std::size_t length,
    const UnicodeRepresentationSize representation_size
)
    : _data(reinterpret_cast<const std::byte*>(data)), _length(length),
      _representation_size(representation_size)
{}

UnicodeView::UnicodeView(const UnicodeStringViewVariant& view_variant)
{
    std::visit(
        [this](const auto& view) {
            this->_data = reinterpret_cast<const std::byte*>(view.data());
            this->_length = view.size();
            using T = std::decay_t<decltype(view)>;
            if constexpr (std::is_same_v<T, std::basic_string_view<char>>) {
                this->_representation_size = UnicodeRepresentationSize::ucs1;
            } else if constexpr (std::is_same_v<
                                     T, std::basic_string_view<char16_t>>) {
                this->_representation_size = UnicodeRepresentationSize::ucs2;
            } else if constexpr (std::is_same_v<
                                     T, std::basic_string_view<char32_t>>) {
                this->_representation_size = UnicodeRepresentationSize::ucs4;
            } else {
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
            }
        },
        view_variant
    );
}

UnicodeView::iterator::iterator(
    const UnicodeView* view, const std::size_t index
)
    : _view(view), _index(index)
{}

UnicodeCodepoint
UnicodeView::iterator::operator*() const
{
    const auto* ptr =
        this->_view->_data +
        this->_index *
            static_cast<std::size_t>(this->_view->_representation_size);
    if (this->_view->_representation_size == UnicodeRepresentationSize::ucs1) {
        return *reinterpret_cast<const char*>(ptr);
    } else if (this->_view->_representation_size ==
               UnicodeRepresentationSize::ucs2) {
        return *reinterpret_cast<const char16_t*>(ptr);
    } else if (this->_view->_representation_size ==
               UnicodeRepresentationSize::ucs4) {
        return *reinterpret_cast<const char32_t*>(ptr);
    }
    throw kaacore::exception("Invalid Unicode representation size");
}

UnicodeView::iterator&
UnicodeView::iterator::operator++()
{
    this->_index++;
    return *this;
}

bool
UnicodeView::iterator::operator==(const iterator& other) const
{
    return this->_index == other._index;
}

bool
UnicodeView::iterator::operator!=(const iterator& other) const
{
    return this->_index != other._index;
}

bool
UnicodeView::operator==(const UnicodeView& other) const
{
    if (this->_length != other._length) {
        return false;
    }
    if (this->_representation_size != other._representation_size) {
        return false;
    }
    return std::equal(
        this->_data,
        this->_data + (this->_length *
                       static_cast<std::size_t>(this->_representation_size)),
        other._data
    );
}

UnicodeView::iterator
UnicodeView::begin() const
{
    return UnicodeView::iterator(this, 0);
}

UnicodeView::iterator
UnicodeView::end() const
{
    return UnicodeView::iterator(this, this->_length);
}

UnicodeStringViewVariant
UnicodeView::string_view_variant() const
{
    if (this->_representation_size == UnicodeRepresentationSize::ucs1) {
        return std::basic_string_view<char>(
            reinterpret_cast<const char*>(this->_data), this->_length
        );
    } else if (this->_representation_size == UnicodeRepresentationSize::ucs2) {
        return std::basic_string_view<char16_t>(
            reinterpret_cast<const char16_t*>(this->_data), this->_length
        );
    } else if (this->_representation_size == UnicodeRepresentationSize::ucs4) {
        return std::basic_string_view<char32_t>(
            reinterpret_cast<const char32_t*>(this->_data), this->_length
        );
    }
    throw kaacore::exception("Invalid Unicode representation size");
}

UnicodeRepresentationSize
UnicodeView::representation_size() const
{
    return this->_representation_size;
}

std::size_t
UnicodeView::length() const
{
    return this->_length;
}

const std::byte*
UnicodeView::data() const
{
    return this->_data;
}

UnicodeBuffer::UnicodeBuffer()
    : _data_container(), _length(0),
      _representation_size(UnicodeRepresentationSize::ucs1)
{}

UnicodeBuffer::UnicodeBuffer(
    const std::byte* data, const std::size_t length,
    const UnicodeRepresentationSize representation_size
)
    : _data_container(length * static_cast<std::size_t>(representation_size)),
      _length(length), _representation_size(representation_size)
{
    std::copy(data, data + length, this->_data_container.data());
}

UnicodeBuffer::UnicodeBuffer(const UnicodeStringViewVariant& view_variant)
{
    std::visit(
        [this](const auto& view) {
            using T = std::decay_t<decltype(view)>;
            if constexpr (std::is_same_v<T, std::basic_string_view<char>>) {
                this->_representation_size = UnicodeRepresentationSize::ucs1;
            } else if constexpr (std::is_same_v<
                                     T, std::basic_string_view<char16_t>>) {
                this->_representation_size = UnicodeRepresentationSize::ucs2;
            } else if constexpr (std::is_same_v<
                                     T, std::basic_string_view<char32_t>>) {
                this->_representation_size = UnicodeRepresentationSize::ucs4;
            } else {
                static_assert(always_false_v<T>, "non-exhaustive visitor!");
            }
            this->_length = view.size();
            this->_data_container = std::vector<std::byte>(
                view.size() *
                static_cast<std::size_t>(this->_representation_size)
            );
            const std::byte* view_data_ptr =
                reinterpret_cast<const std::byte*>(view.data());
            std::copy(
                view_data_ptr,
                view_data_ptr +
                    (view.size() *
                     static_cast<std::size_t>(this->_representation_size)),
                this->_data_container.data()
            );
        },
        view_variant
    );
}

UnicodeBuffer::UnicodeBuffer(const UnicodeView& unicode_view)
    : UnicodeBuffer(unicode_view.string_view_variant())
{}

UnicodeView
UnicodeBuffer::view() const
{
    return UnicodeView(
        this->_data_container.data(), this->_length, this->_representation_size
    );
}

} // namespace kaacore
