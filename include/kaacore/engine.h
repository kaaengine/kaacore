#pragma once

#include <cassert>
#include <memory>

#include <SDL_config.h>
#include <SDL_syswm.h>
#include <SDL.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include "kaacore/renderer.h"


struct Engine {
    SDL_Window* window;
    bgfx::PlatformData platform_data;
    SDL_SysWMinfo wminfo;

    std::unique_ptr<Renderer> renderer;

    Engine();
    ~Engine();
};

extern Engine* engine;


inline Engine* get_engine() {
    assert(::engine != nullptr);
    return ::engine;
}
