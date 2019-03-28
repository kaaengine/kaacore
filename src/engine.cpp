#include <cassert>
#include <memory>

#include <SDL_config.h>
#include <SDL_syswm.h>

#include "kaacore/log.h"
#include "kaacore/engine.h"
#include "kaacore/scenes.h"


namespace kaacore {

Engine* engine;

Engine::Engine() {
    assert(engine == nullptr);

    log<LogLevel::info>("Initializing Kaacore.");
    SDL_Init(SDL_INIT_EVERYTHING);

    engine = this;
}

Engine::~Engine() {
    assert(engine != nullptr);

    log<LogLevel::info>("Shutting down Kaacore.");

    if (this->window) {
        this->input_manager.release();
        this->renderer.release();
        this->window.release();
        bgfx::shutdown();
    }
    SDL_Quit();
    engine = nullptr;
}

Window* Engine::create_window(const std::string& title, int32_t width,
    int32_t height, int32_t x, int32_t y)
{
    this->window = std::make_unique<Window>(
        title, width, height, x, y
    );
    this->_init();
    return this->window.get();
}

SDL_Rect Engine::get_display_rect()
{
    // TODO: add support for multiple displays
    SDL_Rect rect;
    int32_t display_index = 0;
    SDL_GetDisplayBounds(display_index, &rect);
    return rect;
}

void Engine::run(Scene* scene)
{
    if (!this->window) {
        throw std::runtime_error("No window created. Aborting.");
    }

    this->is_running = true;
    log("Engine is running.");

    this->scene = scene;
    uint32_t ticks = SDL_GetTicks();

    while(this->is_running) {
        uint32_t ticks_now = SDL_GetTicks();
        uint32_t dt = ticks_now - ticks;
        ticks = ticks_now;
        this->time += dt;
        this->_pump_events();

        this->renderer->begin_frame();
        this->scene->process_frame(dt);
        this->renderer->end_frame();
    }

    log("Engine stopped.");
}

void Engine::quit() {
    this->is_running = false;
}

void Engine::_init()
{
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(this->window->_window, &wminfo);

#if SDL_VIDEO_DRIVER_X11
    this->platform_data.ndt = wminfo.info.x11.display;
    this->platform_data.nwh = reinterpret_cast<void*>(wminfo.info.x11.window);
#elif SDL_VIDEO_DRIVER_WINDOWS
    this->platform_data.ndt = nullptr;
    this->platform_data.nwh = wminfo.info.win.window;
#else
#error "No platform configuration available for given renderer"
#endif
    this->platform_data.context = nullptr;
    this->platform_data.backBuffer = nullptr;
    this->platform_data.backBufferDS = nullptr;
    bgfx::setPlatformData(this->platform_data);

    bgfx::Init init_data;
    auto window_size = this->window->size();
    init_data.resolution.width = window_size.first;
    init_data.resolution.height = window_size.second;
    init_data.debug = true;

    bgfx::init(init_data);

    this->renderer = std::make_unique<Renderer>();
    this->input_manager = std::make_unique<InputManager>();
}

void Engine::_pump_events()
{
    this->input_manager->clear_events();
    SDL_Event sdl_event;
    while (SDL_PollEvent(&sdl_event)) {
        // TODO handle callbacks
        if (sdl_event.type == SDL_WINDOWEVENT and
            sdl_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            // TODO reset renderer, update camera
            continue;
        }
        this->input_manager->push_event(sdl_event);
    }
}

} // namespace kaacore
