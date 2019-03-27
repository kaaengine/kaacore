#pragma once

#include <string>

#include <SDL.h>

#include <bgfx/platform.h>

#include "kaacore/window.h"
#include "kaacore/renderer.h"


namespace kaacore {

class Scene;
class InputManager;

class Engine {
public:
    uint64_t time = 0;
    Scene* scene = nullptr;
    bool is_running = false;
    bgfx::PlatformData platform_data;

    std::unique_ptr<Window> window = nullptr;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> input_manager;

    Engine();
    ~Engine();

    SDL_Rect get_display_rect();
    void create_window(const std::string& title, int32_t width, int32_t height,
        int32_t x = SDL_WINDOWPOS_UNDEFINED,
        int32_t y = SDL_WINDOWPOS_UNDEFINED, bool fullscreen = false);
    void run(Scene* scene);
    void quit();

private:
    void _init();
    void _pump_events();

};

extern Engine* engine;


inline Engine* get_engine() {
    assert(engine != nullptr);
    return engine;
}

} // namespace kaacore
