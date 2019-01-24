#include <iostream>
#include <memory>

#include <SDL.h>

#include "kaacore/engine.h"

using namespace std;

int main() {
    Engine eng;
    bool running = true;

    auto texture = eng.renderer->default_texture;

    bgfx::setViewClear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH);

    std::vector<StandardVertexData> vertices = {
        {-1., -1., 0.,       0., 1.,     -1., -1.,   0., 1., 1., 1.},
        {1., -1., 0.,        1., 1.,      1., -1.,   1., 0., 1., 1.},
        {1., 1., 0.,         1., 0.,      1.,  1.,   1., 1., 0., 1.},
        {-1., 1., 0.,        0., 0.,     -1.,  1.,   1., 1., 1., 0.}
    };

    std::vector<uint16_t> indices = {0, 2, 1, 0, 3, 2};

    bgfx::setViewRect(0, 0, 0, 800, 600);
    while (running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN and event.key.keysym.sym == SDLK_q) {
                running = false;
                break;
            }
        }
        bgfx::touch(0);
        eng.renderer->render_vertices(vertices, indices, texture);
        bgfx::frame();
    }
}
