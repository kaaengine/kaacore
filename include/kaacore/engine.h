#pragma once

#include <string>

#include <SDL.h>

#include <bgfx/platform.h>
#include <glm/glm.hpp>

#include "kaacore/audio.h"
#include "kaacore/exceptions.h"
#include "kaacore/renderer.h"
#include "kaacore/resources_manager.h"
#include "kaacore/window.h"

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
    uint64_t elapsed_time = 0;
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

    std::unique_ptr<Renderer> _create_renderer();
    void _run(Scene* scene);
    void _swap_scenes();
    void _detach_scenes();
    void _pump_events();
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
    KAACORE_CHECK(is_engine_initialized());
    return engine;
}

} // namespace kaacore
