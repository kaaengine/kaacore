#pragma once

#include <cstdarg>

#include <SDL.h>

#include "kaacore/config.h"

namespace kaacore {

extern bool logging_initialized;

enum class LogLevel {
    verbose = SDL_LOG_PRIORITY_VERBOSE,
    debug = SDL_LOG_PRIORITY_DEBUG,
    info = SDL_LOG_PRIORITY_INFO,
    warn = SDL_LOG_PRIORITY_WARN,
    error = SDL_LOG_PRIORITY_ERROR,
    critical = SDL_LOG_PRIORITY_CRITICAL,
};

enum class LogCategory {
    engine = SDL_LOG_CATEGORY_CUSTOM,
    renderer,
    input,
    audio,
    nodes,
    physics,
    misc,
    application = SDL_LOG_CATEGORY_APPLICATION,
};

inline void
log_dynamic(const LogLevel level, const LogCategory category, const char* msg)
{
#ifndef NDEBUG
    if (not logging_initialized) {
        SDL_LogMessage(
            static_cast<int>(LogCategory::misc), SDL_LOG_PRIORITY_WARN, "%s",
            "Log called with unitialized logging substem.");
    }
#endif

    SDL_LogMessage(
        static_cast<int>(category), static_cast<SDL_LogPriority>(level), "%s",
        msg);
}

inline void
log_dynamic(
    const LogLevel level, const LogCategory category, const char* msg,
    va_list& va)
{
    SDL_LogMessageV(
        static_cast<int>(category), static_cast<SDL_LogPriority>(level), msg,
        va);
}

template<
    LogLevel log_level = LogLevel::info,
    LogCategory log_category = LogCategory::application>
void
log(const char* msg, ...)
{
    va_list va;
    va_start(va, msg);
    log_dynamic(log_level, log_category, msg, va);
    va_end(va);
}

LogLevel
get_logging_level(const LogCategory category);

void
set_logging_level(const LogCategory category, const LogLevel level);

void
initialize_logging();

} // namespace kaacore
