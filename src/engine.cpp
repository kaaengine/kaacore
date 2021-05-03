#include <cassert>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <SDL_config.h>
#include <SDL_syswm.h>

#include "kaacore/audio.h"
#include "kaacore/display.h"
#include "kaacore/exceptions.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/scenes.h"
#include "kaacore/statistics.h"

#include "kaacore/engine.h"

namespace kaacore {

Engine* engine;

constexpr auto threads_sync_timeout = std::chrono::milliseconds(5);

Engine::Engine(
    const glm::uvec2& virtual_resolution,
    const VirtualResolutionMode vr_mode) noexcept(false)
    : _virtual_resolution(virtual_resolution), _virtual_resolution_mode(vr_mode)
{
    KAACORE_CHECK(engine == nullptr, "Engine already initialized.");
    KAACORE_CHECK(
        virtual_resolution.x > 0 and virtual_resolution.y > 0,
        "Virtual resolution must be greater than zero.");
    initialize_logging();
    KAACORE_LOG_INFO("Initializing Kaacore.");
    auto init_flag = SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC |
                     SDL_INIT_GAMECONTROLLER;
    if (SDL_Init(init_flag) < 0) {
        throw kaacore::exception(SDL_GetError());
    }
    this->_main_thread_id = std::this_thread::get_id();
    engine = this;

    this->window = std::make_unique<Window>(this->_virtual_resolution);

    auto bgfx_init_data = this->_gather_platform_data();
    auto window_size = this->window->size();

    this->input_manager = std::make_unique<InputManager>();
    this->audio_manager = std::make_unique<AudioManager>();
#if KAACORE_MULTITHREADING_MODE
    bgfx::renderFrame(); // This marks main thread as "rendering thread"
                         // meaning it will talk with system graphics.
    this->_engine_loop_thread =
        std::thread{[this, bgfx_init_data, window_size]() {
            this->renderer =
                std::make_unique<Renderer>(bgfx_init_data, window_size);
            this->resources_manager = std::make_unique<ResourcesManager>();
            this->_engine_thread_entrypoint();
            // When _engine_thread_entrypoint() exits it means engine is
            // going to stop and it's time to destroy the renderer.
            this->resources_manager.reset();
            this->renderer.reset();
        }};

    // threaded bgfx::init will block until we call bgfx::renderFrame
    while (true) {
        // bgfx needs matching renderFrame() for init to complete
        auto ret = bgfx::renderFrame(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                threads_sync_timeout)
                .count());
        KAACORE_LOG_DEBUG("Waiting for bgfx initialization... ({})", ret);
        if (this->_engine_loop_state.retrieve() !=
            EngineLoopState::not_initialized) {
            break;
        }
    }
#else
    this->renderer = std::make_unique<Renderer>(bgfx_init_data, window_size);
    this->resources_manager = std::make_unique<ResourcesManager>();
#endif
    this->window->show();
    this->udp_stats_exporter = try_make_udp_stats_exporter();
}

Engine::~Engine()
{
    KAACORE_CHECK_TERMINATE(engine != nullptr, "Engine already destroyed.");

    KAACORE_LOG_INFO("Shutting down Kaacore.");
    this->audio_manager.reset();
    this->input_manager.reset();

#if KAACORE_MULTITHREADING_MODE
    this->_engine_loop_state.set(EngineLoopState::terminating);

    while (true) {
        auto ret = bgfx::renderFrame(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                threads_sync_timeout)
                .count());
        if (ret == bgfx::RenderFrame::Enum::Exiting) {
            break;
        }
        KAACORE_LOG_DEBUG("Waiting for bgfx shutdown... ({})", ret);
    }
    this->_engine_loop_thread.join();
#else
    this->resources_manager.reset();
    this->renderer.reset();
#endif

    this->window.reset();
    SDL_Quit();
    engine = nullptr;
}

std::vector<Display>
Engine::get_displays()
{
    return this->make_call_from_main_thread<std::vector<Display>>([this]() {
        std::vector<Display> displays;
        SDL_Rect rect;
        int32_t displays_num = SDL_GetNumVideoDisplays();
        displays.resize(displays_num);
        for (int32_t i = 0; i < displays_num; i++) {
            SDL_GetDisplayUsableBounds(i, &rect);
            Display& display = displays[i];
            display.index = i;
            display.position = {rect.x, rect.y};
            display.size = {rect.w, rect.h};
            display.name = SDL_GetDisplayName(i);
        }

        return displays;
    });
}

void
Engine::run(Scene* scene)
{
    this->_scene = scene;

    this->window->_activate();
#if KAACORE_MULTITHREADING_MODE
    this->_main_thread_entrypoint();
#else
    this->_single_thread_entrypoint();
#endif
    this->window->_deactivate();
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
    KAACORE_CHECK(
        resolution.x > 0 and resolution.y > 0,
        "Virtual resolution must be greater than zero.");
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

bool
Engine::vertical_sync() const
{
    return this->renderer->_vertical_sync;
}

void
Engine::vertical_sync(const bool vsync)
{
    this->renderer->_vertical_sync = vsync;
    this->renderer->reset();
}

double
Engine::get_fps() const
{
    auto duration = this->clock.average_duration();
    if (duration > 0us) {
        return 1.s / duration;
    }
    return 0;
}

Duration
Engine::total_time() const
{
    return this->_total_time;
}

bgfx::Init
Engine::_gather_platform_data()
{
    bgfx::Init bgfx_init_data;
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    SDL_GetWindowWMInfo(this->window->_window, &wminfo);

#if SDL_VIDEO_DRIVER_X11
    // using sdl's provided ndt pointer might cause
    // segfault during engine 2nd initialization,
    // bgfx is capable of querying this info on it's own
    bgfx_init_data.platformData.ndt = nullptr;
    bgfx_init_data.platformData.nwh =
        reinterpret_cast<void*>(wminfo.info.x11.window);
#elif SDL_VIDEO_DRIVER_WINDOWS
    bgfx_init_data.platformData.ndt = nullptr;
    bgfx_init_data.platformData.nwh = wminfo.info.win.window;
#elif SDL_VIDEO_DRIVER_COCOA
    bgfx_init_data.platformData.ndt = nullptr;
    bgfx_init_data.platformData.nwh = wminfo.info.cocoa.window;
#else
#error "No platform configuration available for given renderer"
#endif
    bgfx_init_data.platformData.context = nullptr;
    bgfx_init_data.platformData.backBuffer = nullptr;
    bgfx_init_data.platformData.backBufferDS = nullptr;
    bgfx_init_data.debug = true;
    return bgfx_init_data;
}

void
Engine::_scene_processing()
{
    this->is_running = true;
    try {
        KAACORE_LOG_INFO("Engine is running.");
        this->_scene->on_enter();
        while (this->is_running) {
            auto dt = this->clock.measure();
            {
                StopwatchStatAutoPusher stopwatch{"engine.frame:time"};
                this->renderer->begin_frame();
#if KAACORE_MULTITHREADING_MODE
                this->_event_processing_state.wait(EventProcessingState::ready);
#endif
                this->_process_events();
                if (this->_next_scene) {
                    this->_swap_scenes();
                }
                Duration scaled_dt_sec = dt * this->_scene->_time_scale;
                auto scaled_dt =
                    std::chrono::duration_cast<HighPrecisionDuration>(
                        scaled_dt_sec);
                this->_total_time += scaled_dt_sec;
                {
                    StopwatchStatAutoPusher stopwatch{"scene.update:time"};
                    this->_scene->process_update(scaled_dt_sec);
                }
#if KAACORE_MULTITHREADING_MODE
                this->_event_processing_state.set(
                    EventProcessingState::consumed);
#endif
                const auto& nodes_processing_queue =
                    this->_scene->build_processing_queue();
                this->_scene->update_nodes_drawing_queue(
                    nodes_processing_queue);
                this->_scene->process_drawing();
                this->_scene->resolve_spatial_index_changes(
                    nodes_processing_queue);
                this->_scene->process_physics(scaled_dt);
                this->timers.process(dt);
                this->_scene->timers.process(scaled_dt);
                this->_scene->process_nodes(scaled_dt, nodes_processing_queue);
                this->renderer->end_frame();
                this->_scene->remove_marked_nodes();
            }

            if (this->udp_stats_exporter) {
                this->renderer->push_statistics();
                this->udp_stats_exporter->send_sync(
                    get_global_statistics_manager().get_last_all());
            }
        }
        this->_scene->on_exit();
        KAACORE_LOG_INFO("Engine stopped.");
    } catch (...) {
        this->_detach_scenes();
        this->is_running = false;
        throw;
    }
    this->_detach_scenes();
    this->is_running = false;
}

void
Engine::_swap_scenes()
{
    this->_scene->on_exit();
    this->_next_scene->on_enter();
    this->_scene = std::move(this->_next_scene);
    this->_scene->reset_views();
}

void
Engine::_detach_scenes()
{
    this->_scene.detach();
    this->_next_scene.detach();
}

void
Engine::_process_events()
{
#if !KAACORE_MULTITHREADING_MODE
    SDL_PumpEvents();
#endif
    this->input_manager->clear_events();
    SDL_Event event;
    int peep_status;
    while ((peep_status = SDL_PeepEvents(
                &event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) > 0) {
        if (event.type == EventType::music_finished) {
            this->audio_manager->_handle_music_finished();
        } else if (event.type == EventType::channel_finished) {
            this->audio_manager->_handle_channel_finished(event.user.code);
        } else if (
            event.type == SDL_WINDOWEVENT and
            event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            this->renderer->reset();
            this->_scene->reset_views();
        }
        this->input_manager->push_event(event);
    }
    if (peep_status == -1) {
        throw kaacore::exception(SDL_GetError());
    }
}

#if KAACORE_MULTITHREADING_MODE

void
Engine::_main_thread_entrypoint()
{
    KAACORE_LOG_INFO("Starting main loop.");
    KAACORE_ASSERT(this->_scene, "Running scene not selected.");
    SDL_PumpEvents(); // pump initial events, for 1st update
    this->_event_processing_state.set(EventProcessingState::ready);
    this->_engine_loop_state.set(EngineLoopState::starting);

    while (true) {
        do {
            this->_synced_syscall_queue.finalize_calls();
        } while (this->is_running and
                 not this->_event_processing_state.wait_for(
                     EventProcessingState::consumed, threads_sync_timeout));
        SDL_PumpEvents();
        this->_event_processing_state.set(EventProcessingState::ready);
        do {
            this->_synced_syscall_queue.finalize_calls();
        } while (bgfx::renderFrame(
                     std::chrono::duration_cast<std::chrono::milliseconds>(
                         threads_sync_timeout)
                         .count()) == bgfx::RenderFrame::Enum::Timeout);

        if (this->_engine_loop_state.retrieve() == EngineLoopState::stopping) {
            break;
        }
    }

    if (auto exc_ptr = this->_engine_loop_exception) {
        this->_engine_loop_exception = nullptr;
        std::rethrow_exception(exc_ptr);
    }
}

void
Engine::_engine_thread_entrypoint()
{
    KAACORE_LOG_INFO("Starting engine loop.");
    this->_engine_loop_state.set(EngineLoopState::sleeping);
    KAACORE_LOG_INFO("Starting engine loop: sleeping.");

    while (true) {
        auto retrieved_state = this->_engine_loop_state.wait(
            {EngineLoopState::starting, EngineLoopState::terminating});

        if (retrieved_state == EngineLoopState::terminating) {
            return; // exit from loop so renderer will get terminated
        }

        KAACORE_LOG_INFO("Engine loop is starting to process scenes.");
        try {
            KAACORE_ASSERT(this->_scene, "Running scene not selected.");
            this->_engine_loop_state.set(EngineLoopState::running);
            this->_scene_processing();
        } catch (const std::exception exc) {
            KAACORE_LOG_ERROR(
                "Engine loop interrupted by exception: {}", exc.what());
            this->_engine_loop_exception = std::current_exception();
            KAACORE_LOG_INFO("Engine API loop stopped with exception.");
        }
        this->_engine_loop_state.set(EngineLoopState::stopping);
        KAACORE_LOG_DEBUG("Rendering final frame.");
        // render one more frame to stop waiting renderFrame() from main thread
        this->renderer->end_frame();
        KAACORE_LOG_INFO("Engine loop stopped.");
    }
}

#else

void
Engine::_single_thread_entrypoint()
{
    this->_scene_processing();
}

#endif // KAACORE_MULTITHREADING_MODE

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
    return *this;
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
