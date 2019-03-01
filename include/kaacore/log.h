#pragma once

#include <cstdarg>

#include <SDL.h>


namespace kaacore {

enum class LogLevel {
    debug = SDL_LOG_PRIORITY_DEBUG,
    info = SDL_LOG_PRIORITY_INFO,
    warn = SDL_LOG_PRIORITY_WARN,
    error = SDL_LOG_PRIORITY_ERROR,
    critical = SDL_LOG_PRIORITY_CRITICAL,
};


inline void log_sdl_va(LogLevel level, const char* msg, va_list& va) {
    SDL_LogMessageV(
        SDL_LOG_CATEGORY_APPLICATION,
        static_cast<SDL_LogPriority>(level), msg, va
    );
}


template <LogLevel log_level = LogLevel::info>
void log(const char* msg, ...) {
    va_list va;
    va_start(va, msg);
    log_sdl_va(log_level, msg, va);
    va_end(va);
}

} // namespace kaacore
