#include <string_view>

#include <catch2/catch.hpp>

#include "kaacore/engine.h"

#include "runner.h"

using namespace std::literals::string_view_literals;

TEST_CASE("Test testing framework", "[basics][no_engine]")
{
    REQUIRE(1 == 1);
}

TEST_CASE("Unpack logging settings", "[basics][logging_utils][no_engine]")
{
    SECTION("Empty settings")
    {
        const auto settings = ""sv;
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "") == std::nullopt
        );
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "engine") ==
            std::nullopt
        );
    }

    SECTION("Default value set")
    {
        const auto settings = "info"sv;
        REQUIRE(kaacore::_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "engine") ==
            std::nullopt
        );
    }

    SECTION("Multiple values set")
    {
        const auto settings = "info,engine:warn,renderer:off"sv;
        REQUIRE(kaacore::_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "engine") == "warn"sv
        );
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "renderer") == "off"sv
        );
    }

    SECTION("Multiple values set - with empty sections")
    {
        const auto settings = ",,info,engine:warn,,renderer:off,,"sv;
        REQUIRE(kaacore::_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "engine") == "warn"sv
        );
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "renderer") == "off"sv
        );
    }

    SECTION("Multiple values set - with invalid sections")
    {
        const auto settings =
            "asdf:xxxx,www=11111,info,engine:warn,renderer:off,,"sv;
        REQUIRE(kaacore::_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "engine") == "warn"sv
        );
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "renderer") == "off"sv
        );
    }

    SECTION("Multiple values set - with override")
    {
        const auto settings = "info,engine:warn,renderer:off,engine:trace"sv;
        REQUIRE(kaacore::_unpack_logging_settings(settings, "") == "info"sv);
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "engine") == "trace"sv
        );
        REQUIRE(
            kaacore::_unpack_logging_settings(settings, "renderer") == "off"sv
        );
    }
}

TEST_CASE("Parse compiled file name", "[basics][logging_utils][no_engine]")
{
    REQUIRE(kaacore::_strip_module_name("/absolute/path/file.cpp") == "file");
    REQUIRE(kaacore::_strip_module_name("/absolute/path/file.h") == "file");

    REQUIRE(
        kaacore::_strip_module_name("C:\\absolute\\path\\file.cpp") == "file"
    );
    REQUIRE(
        kaacore::_strip_module_name("C:\\absolute\\path\\file.h") == "file"
    );

    REQUIRE(kaacore::_strip_module_name("relative_path/file.cpp") == "file");
    REQUIRE(kaacore::_strip_module_name("relative_path/file.h") == "file");
    REQUIRE(
        kaacore::_strip_module_name("relative_path/x/y/z/file.cpp") == "file"
    );
    REQUIRE(
        kaacore::_strip_module_name("relative_path/x/y/z/file.h") == "file"
    );

    REQUIRE(kaacore::_strip_module_name("relative_path\\file.cpp") == "file");
    REQUIRE(kaacore::_strip_module_name("relative_path\\file.h") == "file");

    REQUIRE(kaacore::_strip_module_name("file.cpp") == "file");
    REQUIRE(kaacore::_strip_module_name("file.h") == "file");
}

TEST_CASE("Engine start/stop", "[basics][engine_start_stop]")
{
    REQUIRE_FALSE(kaacore::is_engine_initialized());
    {
        auto engine = initialize_testing_engine();
        REQUIRE(kaacore::is_engine_initialized());
    }
    REQUIRE_FALSE(kaacore::is_engine_initialized());
    {
        auto engine = initialize_testing_engine();
        REQUIRE(kaacore::is_engine_initialized());
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
