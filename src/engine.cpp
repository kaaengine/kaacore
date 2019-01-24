#include <cassert>
#include <memory>

#include <SDL_config.h>
#include <SDL_syswm.h>
#include <SDL.h>
#include <bgfx/bgfx.h>

#include "kaacore/log.h"
#include "kaacore/engine.h"
#include "kaacore/renderer.h"


Engine* engine;


Engine::Engine() {
    assert(::engine == nullptr);

    log<LogLevel::info>("Initializing KAAcore engine");
    SDL_Init(SDL_INIT_EVERYTHING);
    this->window = SDL_CreateWindow(
        "KAA window demo", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 800, 600,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    SDL_VERSION(&this->wminfo.version);
    SDL_GetWindowWMInfo(this->window, &wminfo);
    
#if SDL_VIDEO_DRIVER_X11
    this->platform_data.ndt = this->wminfo.info.x11.display;
    this->platform_data.nwh = reinterpret_cast<void*>(this->wminfo.info.x11.window);
#elif SDL_VIDEO_DRIVER_WINDOWS
    this->platform_data.ndt = NULL;
    this->platform_data.nwh = this->wminfo.info.win.window;
#else
#error "No platform configuration available for given renderer"
#endif

    this->platform_data.context = NULL;
    this->platform_data.backBuffer = NULL;
    this->platform_data.backBufferDS = NULL;
    bgfx::setPlatformData(this->platform_data);

    bgfx::Init init_data;
    init_data.resolution.width = 800;
    init_data.resolution.height = 800;
    init_data.debug = true;

#if SDL_VIDEO_DRIVER_WINDOWS
    init_data.type = bgfx::RendererType::Direct3D9;
#endif

    bgfx::init(init_data);

    this->renderer = std::make_unique<Renderer>();

    ::engine = this;
}

Engine::~Engine() {
    assert(::engine != nullptr);

    log<LogLevel::info>("Shutting down KAAcore engine");

    this->renderer.release();
    bgfx::shutdown();
    SDL_DestroyWindow(this->window);
    SDL_Quit();

    ::engine = nullptr;
}
