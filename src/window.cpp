#include "kaacore/engine.h"

#include "kaacore/window.h"

namespace kaacore {

Window::Window(const glm::uvec2& size)
{
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
        this->_show();
    }
    this->_is_shown = true;
}

void
Window::hide()
{
    if (this->_active) {
        SDL_HideWindow(this->_window);
    }
    this->_is_shown = false;
}

std::string
Window::title() const
{
    return SDL_GetWindowTitle(this->_window);
}

void
Window::title(const std::string& title) const
{
    SDL_SetWindowTitle(this->_window, title.c_str());
}

bool
Window::fullscreen() const
{
    return SDL_GetWindowFlags(this->_window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
}

void
Window::fullscreen(const bool fullscreen) const
{
    int32_t value = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(this->_window, value);
}

glm::uvec2
Window::size() const
{
    glm::uvec2 vec;
    SDL_GetWindowSize(
        this->_window, reinterpret_cast<int32_t*>(&vec.x),
        reinterpret_cast<int32_t*>(&vec.y));
    return vec;
}

void
Window::size(const glm::uvec2& size) const
{
    SDL_SetWindowSize(this->_window, size.x, size.y);
}

void
Window::maximize() const
{
    SDL_MaximizeWindow(this->_window);
}

void
Window::minimize() const
{
    SDL_MinimizeWindow(this->_window);
}

void
Window::restore() const
{
    SDL_RestoreWindow(this->_window);
}

glm::uvec2
Window::position() const
{
    glm::uvec2 vec;
    SDL_GetWindowPosition(
        this->_window, reinterpret_cast<int32_t*>(&vec.x),
        reinterpret_cast<int32_t*>(&vec.y));
    return vec;
}

void
Window::position(const glm::uvec2& position) const
{
    SDL_SetWindowPosition(this->_window, position.x, position.y);
}

void
Window::center() const
{
    this->position(glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED));
}

void
Window::_show() const
{
    SDL_ShowWindow(this->_window);
}

void
Window::_activate()
{
    this->_active = true;
    if (this->_is_shown) {
        this->_show();
    }
}

void
Window::_deactivate()
{
    this->hide();
    this->_active = false;
}

} // namespace kaacore
