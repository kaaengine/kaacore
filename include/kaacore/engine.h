#pragma once

#include <cassert>
#include <memory>

#include <SDL_config.h>
#include <SDL_syswm.h>
#include <SDL.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "kaacore/renderer.h"
#include "kaacore/scene.h"
#include "kaacore/input.h"


namespace kaacore {

struct Engine {
    SDL_Window* window;
    bgfx::PlatformData platform_data;
    SDL_SysWMinfo wminfo;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> input_manager;

    Scene* running_scene = nullptr;

    uint64_t time = 0;

    Engine();
    ~Engine();

    void attach_scene(Scene* scene);
    void scene_run();
    void _pump_events();
};

extern Engine* engine;


inline Engine* get_engine() {
    assert(engine != nullptr);
    return engine;
}

} // namespace kaacore
