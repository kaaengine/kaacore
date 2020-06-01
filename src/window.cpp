#include <thread>

#include "kaacore/engine.h"
#include "kaacore/threading.h"

#include "kaacore/window.h"

namespace kaacore {

Window::Window(std::mutex& sdl_windowing_call_mutex, const glm::uvec2& size)
: _sdl_windowing_call_mutex(sdl_windowing_call_mutex)
{
    KAACORE_ASSERT_MAIN_THREAD();
    uint32_t flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    this->_window = SDL_CreateWindow(
        "Kaa", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.x, size.y,
        flags);

    this->size(size);
}

Window::~Window()
{
    SDL_DestroyWindow(this->_window);
}

void
Window::show()
{
    if (this->_active) {
        this->_thread_safe_call([this]() {
            std::lock_guard lock{this->_sdl_windowing_call_mutex};
            SDL_ShowWindow(this->_window);
        });
    }
    this->_is_shown = true;
}

void
Window::hide()
{
    if (this->_active) {
        this->_thread_safe_call([this]() {
            std::lock_guard lock{this->_sdl_windowing_call_mutex};
            SDL_HideWindow(this->_window);
        });
    }
    this->_is_shown = false;
}

std::string
Window::title()
{
    std::lock_guard lock{this->_sdl_windowing_call_mutex};
    return SDL_GetWindowTitle(this->_window);
}

void
Window::title(const std::string& title)
{
    this->_thread_safe_call([this, title]() {
        std::lock_guard lock{this->_sdl_windowing_call_mutex};
        SDL_SetWindowTitle(this->_window, title.c_str());
    });
}

bool
Window::fullscreen()
{
    std::lock_guard lock{this->_sdl_windowing_call_mutex};
    return SDL_GetWindowFlags(this->_window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
}

void
Window::fullscreen(const bool fullscreen)
{
    this->_thread_safe_call([this, fullscreen]() {
        std::lock_guard lock{this->_sdl_windowing_call_mutex};
        int32_t value = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        SDL_SetWindowFullscreen(this->_window, value);
    });
}

glm::uvec2
Window::size()
{
    std::lock_guard lock{this->_sdl_windowing_call_mutex};
    glm::uvec2 vec;
    SDL_GetWindowSize(
        this->_window, reinterpret_cast<int32_t*>(&vec.x),
        reinterpret_cast<int32_t*>(&vec.y));
    return vec;
}

void
Window::size(const glm::uvec2& size)
{
    this->_thread_safe_call([this, size]() {
        std::lock_guard lock{this->_sdl_windowing_call_mutex};
        SDL_SetWindowSize(this->_window, size.x, size.y);
    });
}

void
Window::maximize()
{
    this->_thread_safe_call([this]() {
        std::lock_guard lock{this->_sdl_windowing_call_mutex};
        SDL_MaximizeWindow(this->_window);
    });
}

void
Window::minimize()
{
    this->_thread_safe_call([this]() {
        std::lock_guard lock{this->_sdl_windowing_call_mutex};
        SDL_MinimizeWindow(this->_window);
    });
}

void
Window::restore()
{
    this->_thread_safe_call([this]() {
        std::lock_guard lock{this->_sdl_windowing_call_mutex};
        SDL_RestoreWindow(this->_window);
    });
}

glm::uvec2
Window::position()
{
    std::lock_guard lock{this->_sdl_windowing_call_mutex};
    glm::uvec2 vec;
    SDL_GetWindowPosition(
        this->_window, reinterpret_cast<int32_t*>(&vec.x),
        reinterpret_cast<int32_t*>(&vec.y));
    return vec;
}

void
Window::position(const glm::uvec2& position)
{
    this->_thread_safe_call([this, position]() {
        std::lock_guard lock{this->_sdl_windowing_call_mutex};
        SDL_SetWindowPosition(this->_window, position.x, position.y);
    });
}

void
Window::center()
{
    this->position(glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED));
}

void
Window::_activate()
{
    KAACORE_ASSERT_MAIN_THREAD();
    std::lock_guard lock{this->_sdl_windowing_call_mutex};
    this->_active = true;
    if (this->_is_shown) {
        SDL_ShowWindow(this->_window);
    }
}

void
Window::_deactivate()
{
    KAACORE_ASSERT_MAIN_THREAD();
    std::lock_guard lock{this->_sdl_windowing_call_mutex};
    SDL_HideWindow(this->_window);
    this->_active = false;
}

void
Window::_thread_safe_call(DelayedSyscallFunction&& func)
{
    if (get_engine()->main_thread_id() == std::this_thread::get_id()) {
        log<LogLevel::debug>("Received syscall request... calling now.");
        func();
    } else {
        log<LogLevel::debug>(
            "Received syscall request... not in main thread, delaying.");
        get_engine()->enqueue_syscall_function(std::move(func));
    }
}

} // namespace kaacore
