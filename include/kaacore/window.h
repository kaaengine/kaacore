#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <utility>

#include <SDL.h>
#include <glm/glm.hpp>

#include "kaacore/threading.h"

namespace kaacore {

class Window {
  public:
    Window(const glm::uvec2& size);
    ~Window();

    void show();
    void hide();
    std::string title();
    void title(const std::string& title);
    bool fullscreen();
    void fullscreen(const bool fullscreen);
    glm::uvec2 _peek_size();
    glm::uvec2 size();
    void size(const glm::uvec2& size);
    void maximize();
    void minimize();
    void restore();
    glm::uvec2 position();
    void position(const glm::uvec2& position);
    void center();

  private:
    void _activate();
    void _deactivate();

    bool _active = false;
    bool _is_shown = false;
    SDL_Window* _window = nullptr;

    friend class Engine;
};

} // namespace kaacore
