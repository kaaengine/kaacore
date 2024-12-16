#pragma once

#include <array>
#include <cstdarg>
#include <memory>
#include <optional>
#include <string_view>

#include <SDL.h>

#include <spdlog/pattern_formatter.h>
#include <spdlog/spdlog.h>

#include "kaacore/config.h"
#include "kaacore/utils.h"

using namespace std::literals::string_view_literals;

namespace kaacore {

constexpr std::array _log_categories{
    // all important kaacore modules
    "nodes"sv, "node_ptr"sv, "engine"sv, "files"sv, "log"sv, "renderer"sv,
    "images"sv, "input"sv, "audio"sv, "scenes"sv, "shapes"sv, "physics"sv,
    "resources"sv, "resources_manager"sv, "sprites"sv, "window"sv, "geometry"sv,
    "fonts"sv, "timers"sv, "transitions"sv, "node_transitions"sv, "camera"sv,
    "views"sv, "spatial_index"sv, "threading"sv, "utils"sv, "embedded_data"sv,
    "easings"sv, "shaders"sv, "statistics"sv, "draw_unit"sv, "draw_queue"sv,
    "unicode_buffer"sv,
    // special-purpose categories
    "other"sv, "app"sv, "wrapper"sv, "tools"sv
};

constexpr auto _log_category_fallback =
    find_array_element(_log_categories, "other"sv).value();
constexpr auto _log_category_app =
    find_array_element(_log_categories, "app"sv).value();
constexpr auto _log_category_wrapper =
    find_array_element(_log_categories, "wrapper"sv).value();
constexpr auto _log_category_tools =
    find_array_element(_log_categories, "tools"sv).value();

extern std::array<std::shared_ptr<spdlog::logger>, _log_categories.size()>
    _loggers;
extern bool logging_initialized;

class ConditionalSourceFlag : public spdlog::custom_flag_formatter {
  public:
    std::unique_ptr<custom_flag_formatter> clone() const override;
    void format(
        const spdlog::details::log_msg&, const std::tm&,
        spdlog::memory_buf_t& dest
    ) override;
};

spdlog::level::level_enum
get_logging_level(const std::string_view& category);

void
set_logging_level(
    const std::string_view& category, spdlog::level::level_enum level
);

void
initialize_logging();

inline constexpr std::string_view
_strip_module_name(const std::string_view filename_full)
{
    auto slash_pos = filename_full.find_last_of("/\\");
    auto filename = filename_full.substr(
        slash_pos != std::string_view::npos ? (slash_pos + 1) : 0
    );
    auto dot_pos = filename.rfind('.');
    return filename.substr(0, dot_pos);
}

inline constexpr std::optional<spdlog::level::level_enum>
_parse_logging_level_name(const std::string_view level_name)
{
    if (level_name == "trace") {
        return spdlog::level::trace;
    } else if (level_name == "debug") {
        return spdlog::level::debug;
    } else if (level_name == "info") {
        return spdlog::level::info;
    } else if (level_name == "warn") {
        return spdlog::level::warn;
    } else if (level_name == "error" or level_name == "err") {
        return spdlog::level::err;
    } else if (level_name == "critical") {
        return spdlog::level::critical;
    } else if (level_name == "off") {
        return spdlog::level::off;
    }
    return std::nullopt;
}

inline constexpr std::optional<std::string_view>
_unpack_logging_settings(
    const std::string_view settings, const std::string_view logger_name
)
{
    size_t parser_pos = 0;
    std::string_view section;
    std::optional<std::string_view> found_level;
    while (true) {
        auto next_parser_pos = settings.find(',', parser_pos);
        size_t section_size = 0;
        if (next_parser_pos == std::string_view::npos) {
            section = settings.substr(parser_pos);
        } else {
            section = settings.substr(parser_pos, next_parser_pos - parser_pos);
        }

        if (section != "") {
            auto split_pos = section.find(':');
            // section contains no logger name, treat as default logger
            if (split_pos == std::string_view::npos and logger_name == "") {
                found_level = section;
            }
            // section contains logger name, grab declared level
            else if (split_pos != std::string_view::npos and
                     logger_name != "" and
                     logger_name == section.substr(0, split_pos)) {
                found_level = section.substr(split_pos + 1);
            }
        }

        if (next_parser_pos == std::string_view::npos) {
            break;
        }
        parser_pos = next_parser_pos + 1;
    }

    // return last found matching level name
    return found_level;
}

inline constexpr std::pair<size_t, std::string_view>
_guess_log_category(std::string_view requested_category)
{
    auto found_index = find_array_element(_log_categories, requested_category);
    if (found_index.has_value()) {
        return {found_index.value(), requested_category};
    }
    return {_log_category_fallback, _log_categories[_log_category_fallback]};
}

template<
    spdlog::level::level_enum log_level, size_t logger_index, class... Args>
void
emit_log(spdlog::source_loc source_loc, Args&&... args)
{
    if (not logging_initialized) {
        spdlog::warn("Logging subsystem was not initialized.");
        spdlog::log(source_loc, log_level, std::forward<Args>(args)...);
        return;
    }
    // TODO compile-time cutouts per level
    _loggers[logger_index]->log(
        source_loc, log_level, std::forward<Args>(args)...
    );
}

template<class... Args>
void
emit_log_dynamic(
    spdlog::level::level_enum log_level, size_t logger_index, Args&&... args
)
{
    if (not logging_initialized) {
        spdlog::warn("Logging subsystem was not initialized.");
        spdlog::log(log_level, std::forward<Args>(args)...);
        return;
    }
    _loggers[logger_index]->log(log_level, std::forward<Args>(args)...);
}

#define KAACORE_LOG_FULL(LEVEL, LOGGER_INDEX, ...)                             \
    do {                                                                       \
        kaacore::emit_log<LEVEL, LOGGER_INDEX>(                                \
            spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION},           \
            __VA_ARGS__                                                        \
        );                                                                     \
    } while (0);

#define KAACORE_LOG_AUTO_CATEGORY(LEVEL, ...)                                  \
    do {                                                                       \
        constexpr auto _logger_index =                                         \
            std::get<size_t>(kaacore::_guess_log_category(                     \
                kaacore::_strip_module_name(__FILE__)                          \
            ));                                                                \
        kaacore::emit_log<LEVEL, _logger_index>(                               \
            spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION},           \
            __VA_ARGS__                                                        \
        );                                                                     \
    } while (0);

#define KAACORE_LOG_TRACE(...)                                                 \
    KAACORE_LOG_AUTO_CATEGORY(spdlog::level::trace, __VA_ARGS__)

#define KAACORE_LOG_DEBUG(...)                                                 \
    KAACORE_LOG_AUTO_CATEGORY(spdlog::level::debug, __VA_ARGS__)

#define KAACORE_LOG_INFO(...)                                                  \
    KAACORE_LOG_AUTO_CATEGORY(spdlog::level::info, __VA_ARGS__)

#define KAACORE_LOG_WARN(...)                                                  \
    KAACORE_LOG_AUTO_CATEGORY(spdlog::level::warn, __VA_ARGS__)

#define KAACORE_LOG_ERROR(...)                                                 \
    KAACORE_LOG_AUTO_CATEGORY(spdlog::level::err, __VA_ARGS__)

#define KAACORE_LOG_CRITICAL(...)                                              \
    KAACORE_LOG_AUTO_CATEGORY(spdlog::level::critical, __VA_ARGS__)

#define KAACORE_APP_LOG_TRACE(...)                                             \
    KAACORE_LOG_FULL(spdlog::level::trace, _log_category_app, __VA_ARGS__)

#define KAACORE_APP_LOG_DEBUG(...)                                             \
    KAACORE_LOG_FULL(                                                          \
        spdlog::level::debug, kaacore::_log_category_app, __VA_ARGS__          \
    )

#define KAACORE_APP_LOG_INFO(...)                                              \
    KAACORE_LOG_FULL(                                                          \
        spdlog::level::info, kaacore::_log_category_app, __VA_ARGS__           \
    )

#define KAACORE_APP_LOG_WARN(...)                                              \
    KAACORE_LOG_FULL(                                                          \
        spdlog::level::warn, kaacore::_log_category_app, __VA_ARGS__           \
    )

#define KAACORE_APP_LOG_ERROR(...)                                             \
    KAACORE_LOG_FULL(                                                          \
        spdlog::level::err, kaacore::_log_category_app, __VA_ARGS__            \
    )

#define KAACORE_APP_LOG_CRITICAL(...)                                          \
    KAACORE_LOG_FULL(                                                          \
        spdlog::level::critical, kaacore::_log_category_app, __VA_ARGS__       \
    )

} // namespace kaacore
