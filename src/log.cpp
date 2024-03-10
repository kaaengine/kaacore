#include <cstdlib>
#include <cstring>
#include <string_view>

#include <SDL.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/details/fmt_helper.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "kaacore/exceptions.h"

#include "kaacore/log.h"

namespace kaacore {

bool logging_initialized = false;

std::array<std::shared_ptr<spdlog::logger>, _log_categories.size()> _loggers;

constexpr auto _default_logging_level =
    _parse_logging_level_name(KAACORE_DEFAULT_LOGGING_LEVEL).value();

void
ConditionalSourceFlag::format(const spdlog::details::log_msg &msg, const std::tm &, spdlog::memory_buf_t &dest)
{
    if (msg.source.empty()) {
        return;
    }

    dest.push_back('[');
    std::string_view path = msg.source.filename;
    auto filename = path.substr(path.find_last_of("/\\") + 1);
    spdlog::details::fmt_helper::append_string_view(filename.data(), dest);
    dest.push_back(':');
    spdlog::details::fmt_helper::append_int(msg.source.line, dest);
    dest.push_back(']');
}

std::unique_ptr<spdlog::custom_flag_formatter>
ConditionalSourceFlag::clone() const {
    return std::make_unique<ConditionalSourceFlag>();
}

spdlog::level::level_enum
get_logging_level(const std::string_view& category)
{
    auto found_index = find_array_element(_log_categories, category);
    if (found_index.has_value()) {
        return _loggers[found_index.value()]->level();
    }
    throw kaacore::exception(
        fmt::format("Unknown logging category: {}", category));
}

void
set_logging_level(
    const std::string_view& category, spdlog::level::level_enum level)
{
    auto found_index = find_array_element(_log_categories, category);
    if (found_index.has_value()) {
        _loggers[found_index.value()]->set_level(level);
        return;
    }
    throw kaacore::exception(
        fmt::format("Unknown logging category: {}", category));
}

void
initialize_logging()
{
    if (not logging_initialized) {
        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<ConditionalSourceFlag>('?');
        formatter->set_pattern("%H:%M:%S.%e | %^%-17n %L%$ %v %?");
        spdlog::set_formatter(std::move(formatter));
        const char* logging_settings_env =
            std::getenv("KAACORE_LOGGING_LEVELS");

        auto default_level = _default_logging_level;
        if (logging_settings_env != nullptr) {
            auto requested_default_level_name =
                _unpack_logging_settings(logging_settings_env, "");
            if (requested_default_level_name) {
                spdlog::info(
                    "Found requested default logger level: {}",
                    requested_default_level_name.value());
                auto requested_level = _parse_logging_level_name(
                    requested_default_level_name.value());
                if (requested_level) {
                    default_level = requested_level.value();
                } else {
                    spdlog::warn(
                        "Unrecognized requested default logger level: {}",
                        requested_default_level_name.value());
                }
            }
        }

        for (size_t index = 0; index < _log_categories.size(); index++) {
            auto logger_name = std::string(_log_categories[index]);
            auto logger = spdlog::stderr_color_mt(logger_name);
            std::optional<std::string_view> requested_level_name;
            if (logging_settings_env != nullptr) {
                requested_level_name =
                    _unpack_logging_settings(logging_settings_env, logger_name);
            }
            if (requested_level_name) {
                logger->info(
                    "Found requested logger level: {}",
                    requested_level_name.value());
                auto requested_level =
                    _parse_logging_level_name(requested_level_name.value());
                if (requested_level) {
                    logger->set_level(requested_level.value());
                } else {
                    logger->warn(
                        "Unrecognized requested logger level: {}",
                        requested_level_name.value());
                }
            } else {
                logger->set_level(default_level);
            }
            logger->debug("Initialized new logger (index: {})", index);
            _loggers[index] = logger;
        }

        logging_initialized = true;
    }
}

} // namespace kaacore
