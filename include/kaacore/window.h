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
    Window(const glm::uvec2& size);
    ~Window();

    void show();
    void hide();
    std::string title();
    void title(const std::string& title);
    bool fullscreen();
    void fullscreen(bool fullscreen);
    glm::uvec2 size();
    void size(const glm::uvec2& size);
    void maximize();
    void minimize();
    void restore();
    glm::uvec2 position();
    void position(const glm::uvec2& position);
    void center();

private:
    SDL_Window* _window = nullptr;
};

} // namespace kaacore
