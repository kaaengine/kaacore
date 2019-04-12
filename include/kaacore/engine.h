#pragma once

#include <string>

#include <SDL.h>

#include <bgfx/platform.h>

#include "kaacore/window.h"
#include "kaacore/renderer.h"


namespace kaacore {

class Scene;
struct Display;
class InputManager;

class Engine {
public:
    uint64_t time = 0;
    Scene* scene = nullptr;
    bool is_running = false;
    bgfx::PlatformData platform_data;

    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> input_manager;

    Engine();
    ~Engine();

    std::vector<Display> get_display_info();
    void run(Scene* scene);
    void quit();

private:
    std::unique_ptr<Window> _create_window();
    std::unique_ptr<Renderer> _create_renderer();
    void _pump_events();

};

extern Engine* engine;


inline Engine* get_engine() {
    assert(engine != nullptr);
    return engine;
}

} // namespace kaacore
