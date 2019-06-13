#include "kaacore/engine.h"

#include "kaacore/window.h"


namespace kaacore {

Window::Window(const glm::uvec2& size)
{
    uint32_t flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
    this->_window = SDL_CreateWindow(
        "Kaa", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, size.x, size.y, flags
    );
}

Window::~Window()
{
    SDL_DestroyWindow(this->_window);
}

void Window::show()
{
    SDL_ShowWindow(this->_window);
}

void Window::hide()
{
    SDL_HideWindow(this->_window);
}

std::string Window::title()
{
    return SDL_GetWindowTitle(this->_window);
}

void Window::title(const std::string& title)
{
    SDL_SetWindowTitle(this->_window, title.c_str());
}

bool Window::fullscreen()
{
    return SDL_GetWindowFlags(this->_window) & SDL_WINDOW_FULLSCREEN_DESKTOP;
}

void Window::fullscreen(bool fullscreen)
{
    int32_t value = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(this->_window, value);
}

glm::uvec2 Window::size()
{
    glm::uvec2 vec;
    SDL_GetWindowSize(
        this->_window,
        reinterpret_cast<int32_t*>(&vec.x),
        reinterpret_cast<int32_t*>(&vec.y)
    );
    return vec;
}

void Window::size(const glm::uvec2& size)
{
    SDL_SetWindowSize(this->_window, size.x, size.y);
}

void Window::maximize()
{
    SDL_MaximizeWindow(this->_window);
}

void Window::minimize()
{
    SDL_MinimizeWindow(this->_window);
}

void Window::restore()
{
    SDL_RestoreWindow(this->_window);
}

glm::uvec2 Window::position()
{
    glm::uvec2 vec;
    SDL_GetWindowPosition(
        this->_window,
        reinterpret_cast<int32_t*>(&vec.x),
        reinterpret_cast<int32_t*>(&vec.y)
    );
    return vec;
}

void Window::position(const glm::uvec2& position)
{
    SDL_SetWindowPosition(this->_window, position.x, position.y);
}

void Window::center()
{
    this->position(glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED));
}

} // namespace kaacore
