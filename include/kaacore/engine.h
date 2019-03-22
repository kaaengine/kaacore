#pragma once

#include <cassert>
#include <memory>

#include <SDL_config.h>
#include <SDL_syswm.h>
#include <SDL.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "kaacore/renderer.h"
#include "kaacore/scenes.h"
#include "kaacore/input.h"


namespace kaacore {

struct Engine {
    uint64_t time = 0;
    SDL_Window* window;
    SDL_SysWMinfo wminfo;
    Scene* scene = nullptr;
    bool is_running = false;
    bgfx::PlatformData platform_data;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> input_manager;

    Engine();
    ~Engine();

    void run(Scene* scene);
    void _pump_events();
    void quit();
};

extern Engine* engine;


inline Engine* get_engine() {
    assert(engine != nullptr);
    return engine;
}

} // namespace kaacore
