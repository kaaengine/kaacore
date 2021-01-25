#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <kissnet.hpp>

#include "kaacore/clock.h"

namespace kaacore {

constexpr size_t statistic_tracker_buffer_size = 50u;
constexpr uint16_t udp_stats_exporter_default_port = 9771;
static const char* udp_stats_exporter_env_name = "KAACORE_STATS_EXPORT_UDP";

struct StatisticAnalysis {
    uint32_t samples_count;
    double last_value;
    double mean_value;
    double max_value;
    double min_value;
    double standard_deviation;
};

class FrameStatisticTracker {
  public:
    FrameStatisticTracker();
    void push_value(const double value);
    StatisticAnalysis analyse() const;
    double last_value() const;

  private:
    std::array<double, statistic_tracker_buffer_size> _values_ring_buffer;
    decltype(_values_ring_buffer)::iterator _buffer_end_position;
    decltype(_values_ring_buffer)::iterator _last_value_position;
    decltype(_values_ring_buffer)::iterator _next_value_position;
    size_t _size;
};

class StatisticsManager {
  public:
    void push_value(const std::string& stat_name, const double value);
    std::vector<std::pair<std::string, StatisticAnalysis>> get_report_all();
    std::vector<std::pair<std::string, double>> get_last_all();

  private:
    std::unordered_map<std::string, FrameStatisticTracker> _trackers;
    std::mutex _mutex;
};

class StatAutoPusher {
  protected:
    StatAutoPusher(const std::string& stat_name);
    ~StatAutoPusher() = default;
    StatAutoPusher(const StatAutoPusher&) = delete;
    StatAutoPusher(StatAutoPusher&&) = default;

    StatAutoPusher& operator=(const StatAutoPusher&) = delete;
    StatAutoPusher& operator=(StatAutoPusher&&) = default;

    std::string _stat_name;
};

class CounterStatAutoPusher : public StatAutoPusher {
  public:
    CounterStatAutoPusher(const std::string& stat_name);
    ~CounterStatAutoPusher();

    CounterStatAutoPusher& operator+=(int32_t value);

  private:
    int32_t _counter_value;
};

class StopwatchStatAutoPusher : public StatAutoPusher {
    using StopwatchClock = std::chrono::steady_clock;
    using StopwatchUnit = std::chrono::duration<double>;

  public:
    StopwatchStatAutoPusher(const std::string& stat_name);
    ~StopwatchStatAutoPusher();

  private:
    std::chrono::time_point<StopwatchClock> _start_time;
};

StatisticsManager&
get_global_statistics_manager();

std::vector<std::byte>
pack_stats_data(const std::vector<std::pair<std::string, double>>& stats);

class UDPStatsExporter {
  public:
    UDPStatsExporter(const std::string& endpoint_string);
    void send_sync(const std::vector<std::pair<std::string, double>>& stats);

  private:
    kissnet::udp_socket _socket;
};

std::unique_ptr<UDPStatsExporter>
try_make_udp_stats_exporter();

} // namespace kaacore
