#pragma once

#include <string>
#include <utility>

#include <SDL.h>
#include <glm/glm.hpp>

namespace kaacore {

class Window
{
friend class Engine;
public:
    Window(const std::string& title, int32_t width, int32_t height,
        int32_t x, int32_t y);
    ~Window();

    bool fullscreen();
    void fullscreen(bool fullscreen);
    std::pair<int32_t, int32_t> size();
    void size(const std::pair<int32_t, int32_t>& window_size);
    glm::dvec2 position();
    void position(const glm::dvec2& vec);

private:
    bool _fullscreen = false;
    SDL_Window* _window = nullptr;

};

} // namespace kaacore
