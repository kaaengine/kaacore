#include <string_view>

#include <catch2/catch.hpp>

#include "kaacore/unicode_buffer.h"

#include "runner.h"

TEMPLATE_TEST_CASE(
    "test_create_retrieve_unicode_view", "[unicode_buffer]", char, char16_t,
    char32_t
)
{
    TestType data[] = {'H', 'e', 'l', 'l', 'o', ',', ' ',
                       'w', 'o', 'r', 'l', 'd', '!'};
    kaacore::UnicodeView view{std::basic_string_view<TestType>{data, 13}};

    auto retrieved_string_view =
        std::get<std::basic_string_view<TestType>>(view.string_view_variant());
    REQUIRE(retrieved_string_view.size() == 13);
    REQUIRE(std::equal(data, data + 13, retrieved_string_view.begin()));

    auto index = 0u;
    for (const auto codepoint : view) {
        REQUIRE(codepoint == data[index]);
        index++;
    }
    REQUIRE(index == 13);
}
