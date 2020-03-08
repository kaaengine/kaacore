#include <cassert>
#include <memory>

#include <SDL_config.h>
#include <SDL_syswm.h>

#include "kaacore/audio.h"
#include "kaacore/display.h"
#include "kaacore/exceptions.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/scenes.h"
#include "kaacore/timers.h"

#include "kaacore/engine.h"

namespace kaacore {

Engine* engine;

Engine::Engine(
    const glm::uvec2& virtual_resolution,
    const VirtualResolutionMode vr_mode) noexcept(false)
    : _virtual_resolution(virtual_resolution), _virtual_resolution_mode(vr_mode)
{
    KAACORE_CHECK(engine == nullptr);
    KAACORE_CHECK(virtual_resolution.x > 0 and virtual_resolution.y > 0);
    initialize_logging();

    log<LogLevel::info>("Initializing Kaacore.");
    SDL_Init(SDL_INIT_EVERYTHING);
    engine = this;

    this->window = std::make_unique<Window>(this->_virtual_resolution);
    this->renderer = this->_create_renderer();
    this->input_manager = std::make_unique<InputManager>();
    this->audio_manager = std::make_unique<AudioManager>();
    this->resources_manager = std::make_unique<ResourcesManager>();
}

Engine::~Engine()
{
    KAACORE_CHECK_TERMINATE(engine != nullptr);

    log<LogLevel::info>("Shutting down Kaacore.");
    this->audio_manager.reset();
    this->input_manager.reset();
    this->resources_manager.reset();
    this->renderer.reset();
    bgfx::shutdown();
    this->window.reset();
    destroy_timers();
    SDL_Quit();
    engine = nullptr;
}

std::vector<Display>
Engine::get_displays()
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

void
Engine::run(Scene* scene)
{
    this->is_running = true;
    try {
        this->_run(scene);
    } catch (...) {
        this->_detach_scenes();
        this->is_running = false;
        throw;
    }
    this->_detach_scenes();
    this->is_running = false;
}

void
Engine::_run(Scene* scene)
{
    log("Engine is running.");
    this->_scene = scene;
    this->_scene->on_enter();
    uint32_t ticks = SDL_GetTicks();
    while (this->is_running) {
        uint32_t ticks_now = SDL_GetTicks();
        uint32_t dt = ticks_now - ticks;
        ticks = ticks_now;
        this->elapsed_time += dt;

        this->_pump_events();
        if (this->_next_scene) {
            this->_swap_scenes();
        }
        this->renderer->begin_frame();
        this->_scene->process_frame(dt);
        this->renderer->end_frame();
    }
    this->_scene->on_exit();
    log("Engine stopped.");
}

void
Engine::change_scene(Scene* scene)
{
    this->_next_scene = scene;
}

Scene*
Engine::current_scene()
{
    return this->_scene.data();
}

void
Engine::quit()
{
    this->is_running = false;
}

glm::uvec2
Engine::virtual_resolution() const
{
    return this->_virtual_resolution;
}

void
Engine::virtual_resolution(const glm::uvec2& resolution)
{
    KAACORE_CHECK(resolution.x > 0 and resolution.y > 0);
    this->_virtual_resolution = resolution;
    this->renderer->reset();
}

VirtualResolutionMode
Engine::virtual_resolution_mode() const
{
    return this->_virtual_resolution_mode;
}

void
Engine::virtual_resolution_mode(const VirtualResolutionMode vr_mode)
{
    this->_virtual_resolution_mode = vr_mode;
    this->renderer->reset();
}

std::unique_ptr<Renderer>
Engine::_create_renderer()
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
#elif SDL_VIDEO_DRIVER_COCOA
    this->platform_data.ndt = nullptr;
    this->platform_data.nwh = wminfo.info.cocoa.window;
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

void
Engine::_swap_scenes()
{
    this->_scene->on_exit();
    this->_next_scene->on_enter();
    this->_scene = std::move(this->_next_scene);
}

void
Engine::_detach_scenes()
{
    this->_scene.detach();
    this->_next_scene.detach();
}

void
Engine::_pump_events()
{
    this->input_manager->clear_events();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == EventType::_timer_fired) {
            auto timer_id = reinterpret_cast<TimerID>(event.user.data1);
            resolve_timer(timer_id);
        } else if (event.type == EventType::music_finished) {
            this->audio_manager->_handle_music_finished();
        } else if (event.type == EventType::channel_finished) {
            this->audio_manager->_handle_channel_finished(event.user.code);
        } else if (
            event.type == SDL_WINDOWEVENT and
            event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            this->renderer->reset();
        }
        this->input_manager->push_event(event);
    }
}

Engine::_ScenePointerWrapper::_ScenePointerWrapper() : _scene_ptr(nullptr) {}

Engine::_ScenePointerWrapper::operator bool() const
{
    return this->_scene_ptr != nullptr;
}

Scene* Engine::_ScenePointerWrapper::operator->() const
{
    return this->_scene_ptr;
}

Engine::_ScenePointerWrapper&
Engine::_ScenePointerWrapper::operator=(Scene* const scene)
{
    if (scene == this->_scene_ptr) {
        return *this;
    }

    if (scene) {
        scene->on_attach();
    }

    this->detach();
    this->_scene_ptr = scene;
    return *this;
}

Engine::_ScenePointerWrapper&
Engine::_ScenePointerWrapper::operator=(_ScenePointerWrapper&& other)
{
    if (this == &other) {
        return *this;
    }

    auto tmp = this->_scene_ptr;
    this->_scene_ptr = other._scene_ptr;
    other._scene_ptr = tmp;
    other.detach();
}

void
Engine::_ScenePointerWrapper::detach()
{
    auto prev_scene = this->_scene_ptr;
    this->_scene_ptr = nullptr;
    if (prev_scene) {
        prev_scene->on_detach();
    }
}

Scene*
Engine::_ScenePointerWrapper::data()
{
    return this->_scene_ptr;
}

} // namespace kaacore
