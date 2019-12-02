#include <cstring>

#include <SDL.h>

#include "kaacore/exceptions.h"

#include "kaacore/log.h"

namespace kaacore {

bool logging_initialized = false;

LogLevel
get_logging_level(const LogCategory category)
{
    return static_cast<LogLevel>(
        SDL_LogGetPriority(static_cast<int>(category)));
}

void
set_logging_level(const LogCategory category, const LogLevel level)
{
    SDL_LogSetPriority(
        static_cast<int>(category), static_cast<SDL_LogPriority>(level));
}

void
set_logging_level(const LogCategory category, const char* level_name)
{
    LogLevel level;
    if (strcmp(level_name, "VERBOSE") == 0) {
        level = LogLevel::verbose;
    } else if (strcmp(level_name, "DEBUG") == 0) {
        level = LogLevel::debug;
    } else if (strcmp(level_name, "INFO") == 0) {
        level = LogLevel::info;
    } else if (strcmp(level_name, "WARN") == 0) {
        level = LogLevel::warn;
    } else if (strcmp(level_name, "ERROR") == 0) {
        level = LogLevel::error;
    } else if (strcmp(level_name, "CRITICAL") == 0) {
        level = LogLevel::critical;
    } else {
        log<LogLevel::critical, LogCategory::misc>(
            "Unsupported logging level_name provided: %s", level_name);
        throw exception("Unsupported logging level_name provided");
    }

    set_logging_level(category, level);
}

void
initialize_logging()
{
    if (not logging_initialized) {
        set_logging_level(LogCategory::engine, KAACORE_LOGLEVEL_ENGINE);
        set_logging_level(LogCategory::renderer, KAACORE_LOGLEVEL_RENDERER);
        set_logging_level(LogCategory::input, KAACORE_LOGLEVEL_INPUT);
        set_logging_level(LogCategory::audio, KAACORE_LOGLEVEL_AUDIO);
        set_logging_level(LogCategory::nodes, KAACORE_LOGLEVEL_NODES);
        set_logging_level(LogCategory::physics, KAACORE_LOGLEVEL_PHYSICS);
        set_logging_level(LogCategory::misc, KAACORE_LOGLEVEL_MISC);
        logging_initialized = true;
    }
}

} // namespace kaacore
