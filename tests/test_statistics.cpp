#include <cmath>
#include <tuple>
#include <vector>

#include <catch2/catch.hpp>

#include "kaacore/statistics.h"

#include "runner.h"

using namespace kaacore;
using namespace Catch::literals; // provides literal "_a" (Approx)

TEST_CASE("Test statistics gathering", "[statistics][no_engine]")
{
    FrameStatisticTracker stat_tracker;

    for (auto i = 1; i <= 5; i++) {
        stat_tracker.push_value(i * 1.);
    }

    auto stats = stat_tracker.analyse();
    REQUIRE(stats.samples_count == 5);
    REQUIRE(stats.last_value == 5.);
    REQUIRE(stats.max_value == 5.);
    REQUIRE(stats.min_value == 1.);
    REQUIRE(stats.mean_value == 3);
    REQUIRE(stats.standard_deviation == 1.4142135624_a);
}

TEST_CASE(
    "Test statistics gathering overflow buffer", "[statistics][no_engine]")
{
    FrameStatisticTracker stat_tracker;

    for (auto i = 1; i <= statistic_tracker_buffer_size; i++) {
        stat_tracker.push_value(-10.);
        stat_tracker.push_value(20.);
    }

    auto stats = stat_tracker.analyse();
    REQUIRE(stats.samples_count == statistic_tracker_buffer_size);
    REQUIRE(stats.last_value == 20.);
    REQUIRE(stats.max_value == 20.);
    REQUIRE(stats.min_value == -10.);
}

template<typename T>
T
_parse_type_bytes(std::byte*& data_ptr)
{
    T val = *reinterpret_cast<const T*>(data_ptr);
    data_ptr += sizeof(T);
    return val;
}

std::string
_parse_string(std::byte*& data_ptr, const size_t count)
{
    std::string full_string{reinterpret_cast<const char*>(data_ptr), count};
    data_ptr += count;
    auto null_pos = full_string.find('\0');
    if (null_pos != std::string::npos) {
        return full_string.substr(0, null_pos);
    }
    return full_string;
}

TEST_CASE("Test statistics packing format", "[statistics][no_engine]")
{
    std::vector<std::pair<std::string, double>> sample_stats{
        {"some stat", 15.01},
        {"more stats", 0.00},
        {"stat with very very very very very very very very long name", 13.37}};

    auto packed_message = pack_stats_data(sample_stats);

    auto reader_ptr = packed_message.data();
    REQUIRE(_parse_string(reader_ptr, 12) == "KAACOREstats");
    REQUIRE(_parse_type_bytes<uint16_t>(reader_ptr) == 0x01);
    REQUIRE(_parse_type_bytes<uint16_t>(reader_ptr) == 3);
    reader_ptr += 16; // unused bytes

    REQUIRE(_parse_string(reader_ptr, 40) == "some stat");
    REQUIRE(_parse_type_bytes<double>(reader_ptr) == 15.01);

    REQUIRE(_parse_string(reader_ptr, 40) == "more stats");
    REQUIRE(_parse_type_bytes<double>(reader_ptr) == 0.00);

    // max stat name length is 40
    REQUIRE(
        _parse_string(reader_ptr, 40) ==
        "stat with very very very very very very ");
    REQUIRE(_parse_type_bytes<double>(reader_ptr) == 13.37);
}

TEST_CASE("Test UDPStatsExporter", "[statistics][udp_exporter][no_engine]")
{
    initialize_logging();

    SECTION("Test malformed address")
    {
        REQUIRE_THROWS_AS(
            UDPStatsExporter("invalid_address"), std::runtime_error);
    }

    SECTION("Test sending (custom port)")
    {
        std::vector<std::pair<std::string, double>> sample_stats{
            {"Test sending (custom port)", 1.01}, {"more stats", 0.00}};
        UDPStatsExporter udp_exporter{"127.0.0.1"};
        udp_exporter.send_sync(sample_stats);
    }

    SECTION("Test sending")
    {
        std::vector<std::pair<std::string, double>> sample_stats{
            {"Test sending", 1.01}, {"more stats", 0.00}};
        UDPStatsExporter udp_exporter{"127.0.0.1:1055"};
        udp_exporter.send_sync(sample_stats);
    }

    SECTION("Test sending huge stats amount")
    {
        std::vector<std::pair<std::string, double>> sample_stats = {
            {"Test sending huge stats amount", 1.01},
        };
        for (auto i = 0; i < 1000; i++) {
            sample_stats.emplace_back("huge stats - " + std::to_string(i), i);
        }

        UDPStatsExporter udp_exporter{"127.0.0.1"};
        udp_exporter.send_sync(sample_stats);
    }
}
