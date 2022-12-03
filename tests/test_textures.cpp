#include <catch2/catch.hpp>
#include <glm/gtc/type_precision.hpp>

#include "kaacore/textures.h"

#include "runner.h"

using namespace Catch::Matchers;

TEST_CASE("Test bitmap creation and lookups", "[bitmap]")
{
    kaacore::Bitmap bitmap{{5, 5}};
    for (auto i = 0; i < 5; i++) {
        for (auto j = 0; j < 5; j++) {
            REQUIRE(bitmap.at(i, j) == 0);
        }
    }

    bitmap.at(1, 2) = 100;
    for (auto i = 0; i < 5; i++) {
        for (auto j = 0; j < 5; j++) {
            if (i != 1 or j != 2) {
                REQUIRE(bitmap.at(i, j) == 0);
            } else {
                REQUIRE(bitmap.at(i, j) == 100);
            }
        }
    }
}

TEST_CASE("Test bitmap creation and lookups (4-channel)", "[bitmap]")
{
    kaacore::Bitmap<glm::u8vec4> bitmap{{5, 5}};
    for (auto i = 0; i < 5; i++) {
        for (auto j = 0; j < 5; j++) {
            REQUIRE(bitmap.at(i, j) == glm::u8vec4{0, 0, 0, 0});
        }
    }

    bitmap.at(1, 2) = glm::u8vec4{10, 20, 30, 100};
    for (auto i = 0; i < 5; i++) {
        for (auto j = 0; j < 5; j++) {
            if (i != 1 or j != 2) {
                REQUIRE(bitmap.at(i, j) == glm::u8vec4{0, 0, 0, 0});
            } else {
                REQUIRE(bitmap.at(i, j) == glm::u8vec4{10, 20, 30, 100});
            }
        }
    }
}

TEST_CASE("Test bitmap blitting", "[bitmap]")
{
    kaacore::Bitmap src_bitmap{{3, 3}};
    src_bitmap.at(0, 0) = 10;
    src_bitmap.at(0, 1) = 5;
    src_bitmap.at(1, 0) = 4;
    src_bitmap.at(1, 1) = 20;
    src_bitmap.at(2, 2) = 30;

    SECTION("Blit-copy")
    {
        kaacore::Bitmap bitmap{{3, 3}};
        bitmap.blit(src_bitmap, {0, 0});
        REQUIRE(bitmap.at(0, 0) == 10);
        REQUIRE(bitmap.at(0, 1) == 5);
        REQUIRE(bitmap.at(0, 2) == 0);
        REQUIRE(bitmap.at(1, 0) == 4);
        REQUIRE(bitmap.at(1, 1) == 20);
        REQUIRE(bitmap.at(1, 2) == 0);
        REQUIRE(bitmap.at(2, 0) == 0);
        REQUIRE(bitmap.at(2, 1) == 0);
        REQUIRE(bitmap.at(2, 2) == 30);
    }

    SECTION("Blit-copy overflow")
    {
        kaacore::Bitmap bitmap{{3, 3}};
        REQUIRE_THROWS_WITH(
            bitmap.blit(src_bitmap, {1, 0}), Contains("would overflow X"));

        REQUIRE_THROWS_WITH(
            bitmap.blit(src_bitmap, {0, 1}), Contains("would overflow Y"));
    }

    SECTION("Blit with offset")
    {
        kaacore::Bitmap bitmap{{5, 5}};
        bitmap.blit(src_bitmap, {1, 2});
        REQUIRE(bitmap.at(0, 0) == 0);
        REQUIRE(bitmap.at(0, 1) == 0);
        REQUIRE(bitmap.at(0, 2) == 0);
        REQUIRE(bitmap.at(0, 3) == 0);
        REQUIRE(bitmap.at(0, 4) == 0);
        REQUIRE(bitmap.at(1, 0) == 0);
        REQUIRE(bitmap.at(1, 1) == 0);
        REQUIRE(bitmap.at(1, 2) == 10);
        REQUIRE(bitmap.at(1, 3) == 5);
        REQUIRE(bitmap.at(1, 4) == 0);
        REQUIRE(bitmap.at(2, 0) == 0);
        REQUIRE(bitmap.at(2, 1) == 0);
        REQUIRE(bitmap.at(2, 2) == 4);
        REQUIRE(bitmap.at(2, 3) == 20);
        REQUIRE(bitmap.at(2, 4) == 0);
        REQUIRE(bitmap.at(3, 0) == 0);
        REQUIRE(bitmap.at(3, 1) == 0);
        REQUIRE(bitmap.at(3, 2) == 0);
        REQUIRE(bitmap.at(3, 3) == 0);
        REQUIRE(bitmap.at(3, 4) == 30);
        REQUIRE(bitmap.at(4, 0) == 0);
        REQUIRE(bitmap.at(4, 1) == 0);
        REQUIRE(bitmap.at(4, 2) == 0);
        REQUIRE(bitmap.at(4, 3) == 0);
        REQUIRE(bitmap.at(4, 4) == 0);
    }
}

TEST_CASE("Test texture pixel query", "[texture][query]")
{
    const std::vector<uint8_t> image_content{
        10, 11, 12, 255,
        20, 21, 22, 255,
        30, 31, 32, 255,
        40, 41, 42, 255
    };
    auto image_container = kaacore::load_raw_image(
        bimg::TextureFormat::Enum::RGBA8, 2, 2, image_content);

    REQUIRE(kaacore::query_image_pixel(image_container, {0, 0})
            == glm::dvec4{10 / 255., 11 / 255., 12 / 255., 255 / 255.});
    REQUIRE(kaacore::query_image_pixel(image_container, {1, 0})
            == glm::dvec4{20 / 255., 21 / 255., 22 / 255., 255 / 255.});
    REQUIRE(kaacore::query_image_pixel(image_container, {0, 1})
            == glm::dvec4{30 / 255., 31 / 255., 32 / 255., 255 / 255.});
    REQUIRE(kaacore::query_image_pixel(image_container, {1, 1})
            == glm::dvec4{40 / 255., 41 / 255., 42 / 255., 255 / 255.});
}
