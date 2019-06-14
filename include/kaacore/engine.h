#pragma once

#include <string>

#include <SDL.h>

#include <bgfx/platform.h>
#include <glm/glm.hpp>

#include "kaacore/window.h"
#include "kaacore/renderer.h"
#include "kaacore/audio.h"
#include "kaacore/exceptions.h"


#define KAACORE_DEFAULT_VIRTUAL_RESOLUTION {800, 600}


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
    uint64_t time = 0;
    Scene* scene = nullptr;
    Scene* next_scene = nullptr;
    bool is_running = false;
    bgfx::PlatformData platform_data;

    glm::uvec2 _virtual_resolution;
    VirtualResolutionMode _virtual_resolution_mode = \
        VirtualResolutionMode::adaptive_stretch;

    std::unique_ptr<Window> window;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> input_manager;
    std::unique_ptr<AudioManager> audio_manager;

    Engine(
        const glm::uvec2& virtual_resolution=KAACORE_DEFAULT_VIRTUAL_RESOLUTION,
        const VirtualResolutionMode vr_mode=VirtualResolutionMode::adaptive_stretch
    ) noexcept(false);
    ~Engine() noexcept(false);

    std::vector<Display> get_displays();
    void run(Scene* scene);
    void change_scene(Scene *scene);
    void quit();

    glm::uvec2 virtual_resolution() const;
    void virtual_resolution(const glm::uvec2& resolution);

    VirtualResolutionMode virtual_resolution_mode() const;
    void virtual_resolution_mode(const VirtualResolutionMode vr_mode);

private:
    std::unique_ptr<Window> _create_window();
    std::unique_ptr<Renderer> _create_renderer();
    void _swap_scenes();
    void _pump_events();
};

extern Engine* engine;


inline Engine* get_engine() {
    KAACORE_ASSERT(engine != nullptr);
    return engine;
}

} // namespace kaacore
