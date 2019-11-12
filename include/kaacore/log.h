#pragma once

#include <cstdarg>

#include <SDL.h>

#ifndef KAACORE_LOGLEVEL_DEFAULT
#ifdef NDEBUG
#define KAACORE_LOGLEVEL_DEFAULT "INFO"
#else
#define KAACORE_LOGLEVEL_DEFAULT "DEBUG"
#endif
#endif

#ifndef KAACORE_LOGLEVEL_ENGINE
#define KAACORE_LOGLEVEL_ENGINE KAACORE_LOGLEVEL_DEFAULT
#endif

#ifndef KAACORE_LOGLEVEL_RENDERER
#define KAACORE_LOGLEVEL_RENDERER KAACORE_LOGLEVEL_DEFAULT
#endif

#ifndef KAACORE_LOGLEVEL_INPUT
#define KAACORE_LOGLEVEL_INPUT KAACORE_LOGLEVEL_DEFAULT
#endif

#ifndef KAACORE_LOGLEVEL_AUDIO
#define KAACORE_LOGLEVEL_AUDIO KAACORE_LOGLEVEL_DEFAULT
#endif

#ifndef KAACORE_LOGLEVEL_NODES
#define KAACORE_LOGLEVEL_NODES KAACORE_LOGLEVEL_DEFAULT
#endif

#ifndef KAACORE_LOGLEVEL_PHYSICS
#define KAACORE_LOGLEVEL_PHYSICS KAACORE_LOGLEVEL_DEFAULT
#endif

#ifndef KAACORE_LOGLEVEL_MISC
#define KAACORE_LOGLEVEL_MISC KAACORE_LOGLEVEL_DEFAULT
#endif

namespace kaacore {

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
setup_initial_logging_levels();

} // namespace kaacore
