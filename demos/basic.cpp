#include <iostream>
#include <memory>
#include <utility>

#include <SDL.h>

#include "kaacore/engine.h"

#include "kaacore/images.h"
#include "kaacore/resources.h"
#include "kaacore/utils.h"

using namespace std;
using namespace kaacore;

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({10, 10});
    eng.window->show();
    bool running = true;

    bgfx::TextureHandle texture;
    ResourceReference<Image> res;
    if (argc < 2) {
        texture = eng.renderer->default_texture;
    } else {
        res = Image::load(argv[1]);
        texture = res->texture_handle;
    }

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH);

    std::vector<StandardVertexData> vertices = {
        {-1., -1., 0., 0., 1., -1., -1., 0., 1., 1., 1.},
        {1., -1., 0., 1., 1., 1., -1., 1., 0., 1., 1.},
        {1., 1., 0., 1., 0., 1., 1., 1., 1., 0., 1.},
        {-1., 1., 0., 0., 0., -1., 1., 1., 1., 1., 0.}};

    std::vector<uint16_t> indices = {0, 2, 1, 0, 3, 2};

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN and event.key.keysym.sym == SDLK_q) {
                running = false;
                break;
            }
        }
        eng.renderer->render_vertices(vertices, indices, texture);
        bgfx::frame();
    }

    return 0;
}
