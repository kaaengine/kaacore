#include <thread>

#include "kaacore/engine.h"
#include "kaacore/threading.h"

#include "kaacore/window.h"

namespace kaacore {

Window::Window(const glm::uvec2& size)
{
    KAACORE_ASSERT_MAIN_THREAD();
    uint32_t flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    this->_window = SDL_CreateWindow(
        "Kaa", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.x, size.y,
        flags
    );

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
        get_engine()->make_call_from_main_thread<void>([this]() {
            SDL_ShowWindow(this->_window);
        });
    }
    this->_is_shown = true;
}

void
Window::hide()
{
    if (this->_active) {
        get_engine()->make_call_from_main_thread<void>([this]() {
            SDL_HideWindow(this->_window);
        });
    }
    this->_is_shown = false;
}

std::string
Window::title()
{
    return get_engine()->make_call_from_main_thread<std::string>([this]() {
        return SDL_GetWindowTitle(this->_window);
    });
}

void
Window::title(const std::string& title)
{
    get_engine()->make_call_from_main_thread<void>([this, title]() {
        SDL_SetWindowTitle(this->_window, title.c_str());
    });
}

bool
Window::fullscreen()
{
    return get_engine()->make_call_from_main_thread<bool>([this]() {
        return SDL_GetWindowFlags(this->_window) &
               SDL_WINDOW_FULLSCREEN_DESKTOP;
    });
}

void
Window::fullscreen(const bool fullscreen)
{
    get_engine()->make_call_from_main_thread<void>([this, fullscreen]() {
        int32_t value = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        SDL_SetWindowFullscreen(this->_window, value);
    });
}

glm::uvec2
Window::_peek_size()
{
    glm::uvec2 vec;
    SDL_GetWindowSize(
        this->_window, reinterpret_cast<int32_t*>(&vec.x),
        reinterpret_cast<int32_t*>(&vec.y)
    );
    return vec;
}

glm::uvec2
Window::size()
{
    return get_engine()->make_call_from_main_thread<glm::uvec2>([this]() {
        return this->_peek_size();
    });
}

void
Window::size(const glm::uvec2& size)
{
    get_engine()->make_call_from_main_thread<void>([this, size]() {
        SDL_SetWindowSize(this->_window, size.x, size.y);
    });
}

void
Window::maximize()
{
    get_engine()->make_call_from_main_thread<void>([this]() {
        SDL_MaximizeWindow(this->_window);
    });
}

void
Window::minimize()
{
    get_engine()->make_call_from_main_thread<void>([this]() {
        SDL_MinimizeWindow(this->_window);
    });
}

void
Window::restore()
{
    get_engine()->make_call_from_main_thread<void>([this]() {
        SDL_RestoreWindow(this->_window);
    });
}

glm::uvec2
Window::position()
{
    return get_engine()->make_call_from_main_thread<glm::uvec2>([this]() {
        glm::uvec2 vec;
        SDL_GetWindowPosition(
            this->_window, reinterpret_cast<int32_t*>(&vec.x),
            reinterpret_cast<int32_t*>(&vec.y)
        );
        return vec;
    });
}

void
Window::position(const glm::uvec2& position)
{
    get_engine()->make_call_from_main_thread<void>([this, position]() {
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
    this->_active = true;
    if (this->_is_shown) {
        SDL_ShowWindow(this->_window);
    }
}

void
Window::_deactivate()
{
    KAACORE_ASSERT_MAIN_THREAD();
    SDL_HideWindow(this->_window);
    this->_active = false;
}

} // namespace kaacore
