#include <cassert>
#include <memory>

#include <SDL_config.h>
#include <SDL_syswm.h>

#include "kaacore/log.h"
#include "kaacore/engine.h"
#include "kaacore/scenes.h"
#include "kaacore/display.h"
#include "kaacore/exceptions.h"

#include "kaacore/engine.h"


namespace kaacore {

Engine* engine;

Engine::Engine() {
    KAACORE_CHECK(engine == nullptr);

    log<LogLevel::info>("Initializing Kaacore.");
    SDL_Init(SDL_INIT_EVERYTHING);
    engine = this;

    this->window = this->_create_window();
    this->renderer = this->_create_renderer();
    this->input_manager = std::make_unique<InputManager>();
}

Engine::~Engine() {
    KAACORE_CHECK(engine != nullptr);

    log<LogLevel::info>("Shutting down Kaacore.");
    this->input_manager.release();
    this->renderer.release();
    bgfx::shutdown();
    this->window.release();
    SDL_Quit();
    engine = nullptr;
}

std::vector<Display> Engine::get_displays()
{
    SDL_Rect rect;
    std::vector<Display> result;
    int32_t displays_num = SDL_GetNumVideoDisplays();
    result.resize(displays_num);
    for (int32_t i = 0; i < displays_num; i++) {
        SDL_GetDisplayUsableBounds(i, &rect);
        Display& display = result[i];
        display.index = i;
        display.position = {rect.x, rect.y};
        display.size = {rect.w, rect.h};
        display.name = SDL_GetDisplayName(i);
    }
    return result;
}

void Engine::run(Scene* scene)
{
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

std::unique_ptr<Window> Engine::_create_window()
{
    Display display = this->get_displays().at(0);
    return std::make_unique<Window>(display.size * 2u / 3u);
}

std::unique_ptr<Renderer> Engine::_create_renderer()
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
    init_data.resolution.width = window_size.x;
    init_data.resolution.height = window_size.y;
    init_data.debug = true;

    bgfx::init(init_data);

    return std::make_unique<Renderer>(this->window->size());
}

void Engine::_pump_events()
{
    this->input_manager->clear_events();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        // TODO handle callbacks
        if (event.type == SDL_WINDOWEVENT and
            event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            this->renderer->reset(event.window.data1, event.window.data2);
            // TODO update camera
            continue;
        }
        this->input_manager->push_event(event);
    }
}

} // namespace kaacore
