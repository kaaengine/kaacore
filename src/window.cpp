#include "kaacore/log.h"
#include "kaacore/window.h"


namespace kaacore {

Window::Window(const std::string& title, int32_t width, int32_t height,
    int32_t x, int32_t y, bool fullscreen) : _fullscreen(fullscreen)
{
    uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    this->_window = SDL_CreateWindow(title.c_str(), x, y, width, height, flags);
}

Window::~Window()
{
    SDL_DestroyWindow(this->_window);
}

bool Window::fullscreen()
{
    return this->_fullscreen;
}

void Window::fullscreen(bool fullscreen)
{
    this->_fullscreen = fullscreen;
    if (fullscreen) {
        SDL_SetWindowFullscreen(this->_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(this->_window, 0);
    }
}

std::pair<int32_t, int32_t> Window::size()
{
    int32_t w, h;
    SDL_GetWindowSize(this->_window, &w, &h);
    return {w, h};
}

} // namespace kaacore
