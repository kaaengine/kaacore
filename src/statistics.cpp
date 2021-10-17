#include <algorithm>
#include <chrono>
#include <cmath>
#include <mutex>

#include "kaacore/engine.h"

#include "kaacore/statistics.h"

namespace kaacore {

FrameStatisticTracker::FrameStatisticTracker() : _size(0u)
{
    this->_buffer_end_position = this->_values_ring_buffer.begin();
    this->_next_value_position = this->_values_ring_buffer.begin();
    this->_last_value_position = this->_values_ring_buffer.begin();
    *this->_last_value_position = std::nan("");
}

void
FrameStatisticTracker::push_value(const double value)
{
    *this->_next_value_position = value;
    this->_last_value_position = this->_next_value_position;
    this->_next_value_position++;
    if (this->_next_value_position == this->_values_ring_buffer.end()) {
        this->_next_value_position = this->_values_ring_buffer.begin();
    }
    if (this->_buffer_end_position != this->_values_ring_buffer.end()) {
        this->_buffer_end_position++;
    }
}

double
FrameStatisticTracker::last_value() const
{
    return *this->_last_value_position;
}

StatisticAnalysis
FrameStatisticTracker::analyse() const
{
    StatisticAnalysis stats;
    stats.samples_count =
        this->_buffer_end_position - this->_values_ring_buffer.begin();
    stats.last_value = *this->_last_value_position;
    stats.max_value = *this->_last_value_position;
    stats.min_value = *this->_last_value_position;

    const auto start_it = this->_values_ring_buffer.begin();
    const auto end_it = this->_buffer_end_position;

    if (stats.samples_count) {
        double sum = 0.;
        for (auto it = start_it; it < end_it; it++) {
            if (*it > stats.max_value) {
                stats.max_value = *it;
            }
            if (*it < stats.min_value) {
                stats.min_value = *it;
            }
            sum += *it;
        }

        stats.mean_value = sum / stats.samples_count;
        double variance_sum = 0.;

        for (auto it = this->_values_ring_buffer.begin();
             it < this->_buffer_end_position; it++) {
            if (not std::isnan(*it)) {
                double mean_error = *it - stats.mean_value;
                variance_sum += mean_error * mean_error;
            }
        }

        stats.standard_deviation =
            std::sqrt(variance_sum / stats.samples_count);
    } else {
        stats.mean_value = std::nan("");
        stats.standard_deviation = std::nan("");
    }

    return stats;
}

std::optional<StatisticAnalysis>
StatisticsManager::get_analysis(const std::string& name)
{
    std::lock_guard lock{this->_mutex};
    auto it = this->_trackers.find(name);
    if (it != this->_trackers.end()) {
        return it->second.analyse();
    }
    return std::nullopt;
}

void
StatisticsManager::push_value(const std::string& stat_name, const double value)
{
    std::lock_guard lock{this->_mutex};
    auto [iter, inserted] = this->_trackers.try_emplace(stat_name);
    iter->second.push_value(value);
}

std::vector<std::pair<std::string, StatisticAnalysis>>
StatisticsManager::get_analysis_all()
{
    std::vector<std::pair<std::string, StatisticAnalysis>> report;
    std::lock_guard lock{this->_mutex};

    for (const auto& [stat_name, tracker] : this->_trackers) {
        report.emplace_back(stat_name, tracker.analyse());
    }

    return report;
}

std::vector<std::pair<std::string, double>>
StatisticsManager::get_last_all()
{
    std::vector<std::pair<std::string, double>> last_values;
    std::lock_guard lock{this->_mutex};

    for (const auto& [stat_name, tracker] : this->_trackers) {
        last_values.emplace_back(stat_name, tracker.last_value());
    }

    return last_values;
}

inline void
_push_stat_to_manager(const std::string& stat_name, const double value)
{
    get_global_statistics_manager().push_value(stat_name, value);
}

StatAutoPusher::StatAutoPusher(const std::string& stat_name)
    : _stat_name(stat_name)
{}

CounterStatAutoPusher::CounterStatAutoPusher(const std::string& stat_name)
    : StatAutoPusher(stat_name), _counter_value(0)
{}

CounterStatAutoPusher::~CounterStatAutoPusher()
{
    _push_stat_to_manager(this->_stat_name, this->_counter_value);
}

CounterStatAutoPusher&
CounterStatAutoPusher::operator+=(int32_t value)
{
    this->_counter_value += value;
    return *this;
}

StopwatchStatAutoPusher::StopwatchStatAutoPusher(const std::string& stat_name)
    : StatAutoPusher(stat_name), _start_time(StopwatchClock::now())
{}

StopwatchStatAutoPusher::~StopwatchStatAutoPusher()
{
    StopwatchUnit time_delta = StopwatchClock::now() - this->_start_time;
    _push_stat_to_manager(this->_stat_name, time_delta.count());
}

StatisticsManager&
get_global_statistics_manager()
{
    static StatisticsManager statistics_manager;
    return statistics_manager;
}

std::vector<std::byte>
pack_stats_data(const std::vector<std::pair<std::string, double>>& stats)
{
    /* KAACORE stats packing format
     * bytes        description
     * === Message format ===
     * 12           magic bytes, literal: "KAACOREstats"
     *  2           version, literal: 0x01
     *  2           stat segments count, uint
     * 16           reserved, unused
     * 48*N         N stat segments, N = stat segments count
     * === Stat segment format ===
     * 40           name of stat, string, padded with NULL
     *  8           stat value, double
     */

    const uint16_t pack_version = 0x01;
    const uint16_t segments_count = stats.size();
    const size_t header_size = 32;
    const size_t stat_segment_size = 48;

    size_t write_offset = 0;
    std::vector<std::byte> packed_message;
    packed_message.resize(
        header_size + stat_segment_size * segments_count, std::byte{'\0'});

    auto do_write = [&write_offset,
                     &packed_message](const void* src, const size_t count) {
        KAACORE_ASSERT(
            write_offset + count <= packed_message.size(),
            "Write outside message bounds, requested: {}, max: {}.",
            write_offset + count, packed_message.size());
        std::memcpy(packed_message.data() + write_offset, src, count);
        write_offset += count;
    };

    do_write("KAACOREstats", 12);

    static_assert(sizeof(pack_version) == 2, "Invalid size of `pack_version`");
    do_write(&pack_version, 2);

    static_assert(
        sizeof(segments_count) == 2, "Invalid size of `segments_count`");
    do_write(&segments_count, 2);

    // RESERVED
    write_offset += 16;

    for (const auto& [stat_name, value] : stats) {
        const size_t name_size = std::min<size_t>(40u, stat_name.size());
        do_write(stat_name.data(), name_size);
        write_offset += 40 - name_size;

        static_assert(sizeof(value) == 8, "Invalid size of `value`");
        do_write(&value, 8);
    }

    return packed_message;
}

kissnet::endpoint
_parse_endpoint(const std::string& endpoint_string)
{
    const auto separator = endpoint_string.find_last_of(':');
    if (separator == std::string::npos) {
        return kissnet::endpoint{endpoint_string,
                                 udp_stats_exporter_default_port};
    } else {
        return kissnet::endpoint{endpoint_string};
    }
}

UDPStatsExporter::UDPStatsExporter(const std::string& endpoint_string)
    : _socket(_parse_endpoint(endpoint_string))
{
    KAACORE_LOG_INFO(
        "Started UDP stats exporter (exporting to: {})", endpoint_string);
}

void
UDPStatsExporter::send_sync(
    const std::vector<std::pair<std::string, double>>& stats)
{
    auto packed_stats = pack_stats_data(stats);

    auto [size, status] =
        this->_socket.send(packed_stats.data(), packed_stats.size());
    KAACORE_LOG_TRACE(
        "Sent bytes: {} of {}, status: {}", size, packed_stats.size(),
        status.value);
}

std::unique_ptr<UDPStatsExporter>
try_make_udp_stats_exporter()
{
    std::unique_ptr<UDPStatsExporter> stats_exporter{};
    if (const char* udp_address_raw =
            std::getenv(udp_stats_exporter_env_name)) {
        const std::string udp_address{udp_address_raw};
        if (not udp_address.empty()) {
            stats_exporter = std::make_unique<UDPStatsExporter>(udp_address);
        }
    }

    return std::move(stats_exporter);
}

} // namespace kaacore
