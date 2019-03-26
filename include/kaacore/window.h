#pragma once

#include <string>
#include <utility>

#include <SDL.h>

namespace kaacore {

class Window
{
friend class Engine;
public:
    Window(std::string title, int32_t width, int32_t height,
        int32_t x, int32_t y, bool fullscreen);
    ~Window();

    bool fullscreen();
    void fullscreen(const bool fullscreen);
    std::pair<int32_t, int32_t> size();
    // TODO: setting window size, fullscreen modes, etc.

private:
    bool _fullscreen;
    SDL_Window* _window = nullptr;

};

} // namespace kaacore
