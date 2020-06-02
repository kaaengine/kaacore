#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

#include <SDL.h>

#include <bgfx/platform.h>
#include <glm/glm.hpp>

#include "kaacore/audio.h"
#include "kaacore/config.h"
#include "kaacore/exceptions.h"
#include "kaacore/renderer.h"
#include "kaacore/resources_manager.h"
#include "kaacore/threading.h"
#include "kaacore/window.h"

#define KAACORE_ASSERT_MAIN_THREAD()                                           \
    do {                                                                       \
        KAACORE_ASSERT(                                                        \
            get_engine()->main_thread_id() == std::this_thread::get_id());     \
    } while (0)

namespace kaacore {

class Scene;
struct Display;
class InputManager;

enum struct VirtualResolutionMode {
    adaptive_stretch = 1,
    aggresive_stretch = 2,
    no_stretch = 3,
};

class Engine {
  public:
    bool is_running = false;
    bgfx::PlatformData platform_data;

    glm::uvec2 _virtual_resolution;
    VirtualResolutionMode _virtual_resolution_mode =
        VirtualResolutionMode::adaptive_stretch;

    // use pointers so we can have more controll over destruction order
    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> input_manager;
    std::unique_ptr<AudioManager> audio_manager;
    std::unique_ptr<ResourcesManager> resources_manager;

    Engine(
        const glm::uvec2& virtual_resolution,
        const VirtualResolutionMode vr_mode =
            VirtualResolutionMode::adaptive_stretch) noexcept(false);
    ~Engine();

    std::vector<Display> get_displays();
    void run(Scene* scene);
    void change_scene(Scene* scene);
    Scene* current_scene();
    void quit();

    glm::uvec2 virtual_resolution() const;
    void virtual_resolution(const glm::uvec2& resolution);

    VirtualResolutionMode virtual_resolution_mode() const;
    void virtual_resolution_mode(const VirtualResolutionMode vr_mode);

    inline std::thread::id main_thread_id() { return this->_main_thread_id; }

    inline std::thread::id engine_thread_id()
    {
#if KAACORE_MULTITHREADING_MODE
        return this->_engine_thread_id;
#else
        return this->_main_thread_id;
#endif
    }

    void enqueue_syscall_function(DelayedSyscallFunction&& func);

  private:
    class _ScenePointerWrapper {
      public:
        _ScenePointerWrapper();
        _ScenePointerWrapper(const _ScenePointerWrapper&) = delete;
        _ScenePointerWrapper(const _ScenePointerWrapper&&) = delete;
        _ScenePointerWrapper& operator=(const _ScenePointerWrapper&) = delete;
        _ScenePointerWrapper& operator=(Scene* const scene);
        _ScenePointerWrapper& operator=(_ScenePointerWrapper&& other);
        operator bool() const;
        Scene* operator->() const;
        void detach();
        Scene* data();

      private:
        Scene* _scene_ptr;
    };

    _ScenePointerWrapper _scene;
    _ScenePointerWrapper _next_scene;
    std::vector<Display> _displays;

    std::thread::id _main_thread_id;
    DelayedSyscallQueue _delayed_syscall_queue;
    std::mutex _sdl_windowing_call_mutex;

#if KAACORE_MULTITHREADING_MODE
    enum struct EngineLoopState {
        not_initialized = 1,
        sleeping = 2,
        starting = 11,
        running = 12,
        stopping = 13,
        terminating = 21,
        terminated = 22,
    };

    enum struct EventProcessingState {
        not_initialized = 1,
        ready = 2,
        consumed = 3,
    };

    AwaitableStateEnum<EngineLoopState> _engine_loop_state =
        EngineLoopState::not_initialized;
    AwaitableStateEnum<EventProcessingState> _event_processing_state =
        EventProcessingState::not_initialized;
    std::thread _engine_loop_thread;
    std::thread::id _engine_thread_id;
    std::exception_ptr _engine_loop_exception;
#endif

    bgfx::Init _gather_platform_data();
    void _refresh_displays();
    void _scene_processing();
    void _scene_processing_single();
    void _swap_scenes();
    void _detach_scenes();
    void _process_events();

#if KAACORE_MULTITHREADING_MODE
    void _main_loop_thread_entrypoint();
    void _engine_loop_thread_entrypoint();
#else
    void _single_thread_entrypoint();
#endif
};

extern Engine* engine;

inline bool
is_engine_initialized()
{
    return engine != nullptr;
}

inline Engine*
get_engine()
{
    KAACORE_CHECK(is_engine_initialized(), "Engine is not initialized.");
    return engine;
}

} // namespace kaacore
