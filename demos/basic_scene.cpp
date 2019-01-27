#include <iostream>
#include <memory>

#include <SDL.h>

#include "kaacore/engine.h"
#include "kaacore/scene.h"
#include "kaacore/log.h"
#include "kaacore/input.h"


struct DemoScene : Scene {
    DemoScene()
    {
        bgfx::setViewRect(0, 0, 0, 800, 600);
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH);
    }

    void update(uint32_t dt) override
    {
        log<LogLevel::debug>("DemoScene update %lu/%llu", dt, this->time);
        auto texture = get_engine()->renderer->default_texture;


        std::vector<StandardVertexData> vertices = {
            {-1., -1., 0.,       0., 1.,     -1., -1.,   0., 1., 1., 1.},
            {1., -1., 0.,        1., 1.,      1., -1.,   1., 0., 1., 1.},
            {1., 1., 0.,         1., 0.,      1.,  1.,   1., 1., 0., 1.},
            {-1., 1., 0.,        0., 0.,     -1.,  1.,   1., 1., 1., 0.}
        };

        std::vector<uint16_t> indices = {0, 2, 1, 0, 3, 2};

        for (auto const& event : this->get_events()) {
            if (event.is_pressing(Keycode::kc_q) or event.is_quit()) {
                get_engine()->attach_scene(nullptr);
                break;
            }
        }
        get_engine()->renderer->render_vertices(vertices, indices, texture);
    }
};


extern "C" int main(int argc, char *argv[])
{
    Engine eng;
    DemoScene scene;
    eng.attach_scene(&scene);
    eng.scene_run();

    return 0;
}
