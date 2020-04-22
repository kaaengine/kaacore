#pragma once

#include <string>
#include <utility>

#include <SDL.h>
#include <glm/glm.hpp>

namespace kaacore {

class Window {
  public:
    Window(const glm::uvec2& size);
    ~Window();

    void show();
    void hide();
    std::string title() const;
    void title(const std::string& title) const;
    bool fullscreen() const;
    void fullscreen(const bool fullscreen) const;
    glm::uvec2 size() const;
    void size(const glm::uvec2& size) const;
    void maximize() const;
    void minimize() const;
    void restore() const;
    glm::uvec2 position() const;
    void position(const glm::uvec2& position) const;
    void center() const;

  private:
    bool _active = false;
    bool _is_shown = false;
    SDL_Window* _window = nullptr;

    void _show() const;
    void _activate();
    void _deactivate();

    friend class Engine;
};

} // namespace kaacore
