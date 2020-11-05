#include <string_view>

#include <catch2/catch.hpp>

#include "kaacore/engine.h"

#include "runner.h"

using namespace std::literals::string_view_literals;
using namespace kaacore;

TEST_CASE("Test testing framework", "[basics]")
{
    REQUIRE(1 == 1);
}

TEST_CASE("Unpack logging settings", "[basics][logging_utils]")
{
    SECTION("Empty settings")
    {
        const auto settings = ""sv;
        REQUIRE(_unpack_logging_settings(settings, "") == std::nullopt);
        REQUIRE(_unpack_logging_settings(settings, "engine") == std::nullopt);
    }

    SECTION("Default value set")
    {
        const auto settings = "info"sv;
        REQUIRE(_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(_unpack_logging_settings(settings, "engine") == std::nullopt);
    }

    SECTION("Multiple values set")
    {
        const auto settings = "info,engine:warn,renderer:off"sv;
        REQUIRE(_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(_unpack_logging_settings(settings, "engine") == "warn"sv);
        REQUIRE(_unpack_logging_settings(settings, "renderer") == "off"sv);
    }

    SECTION("Multiple values set - with empty sections")
    {
        const auto settings = ",,info,engine:warn,,renderer:off,,"sv;
        REQUIRE(_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(_unpack_logging_settings(settings, "engine") == "warn"sv);
        REQUIRE(_unpack_logging_settings(settings, "renderer") == "off"sv);
    }

    SECTION("Multiple values set - with invalid sections")
    {
        const auto settings =
            "asdf:xxxx,www=11111,info,engine:warn,renderer:off,,"sv;
        REQUIRE(_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(_unpack_logging_settings(settings, "engine") == "warn"sv);
        REQUIRE(_unpack_logging_settings(settings, "renderer") == "off"sv);
    }

    SECTION("Multiple values set - with override")
    {
        const auto settings = "info,engine:warn,renderer:off,engine:trace"sv;
        REQUIRE(_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(_unpack_logging_settings(settings, "engine") == "trace"sv);
        REQUIRE(_unpack_logging_settings(settings, "renderer") == "off"sv);
    }
}

TEST_CASE("Engine start/stop", "[basics][engine_start_stop]")
{
    REQUIRE_FALSE(is_engine_initialized());
    {
        auto engine = initialize_testing_engine();
        REQUIRE(is_engine_initialized());
    }
    REQUIRE_FALSE(is_engine_initialized());
    {
        auto engine = initialize_testing_engine();
        REQUIRE(is_engine_initialized());
    }
}

TEST_CASE("Testing scene example usage", "[basics]")
{
    auto engine = initialize_testing_engine();
    uint32_t frames_counter = 0;

    TestingScene scene;
    scene.update_function = [&frames_counter](auto dt) { frames_counter++; };
    scene.run_on_engine(10);
    REQUIRE(frames_counter == 10);
}
