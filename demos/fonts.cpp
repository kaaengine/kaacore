#include <vector>
#include <iostream>
#include <memory>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"
#include "kaacore/log.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"
#include "kaacore/fonts.h"

using namespace kaacore;


struct DemoFontsScene : Scene {
    Node* node_text_raw;

    DemoFontsScene() {
        this->camera.size = {800., 600.};
        this->camera.refresh();

        auto font = Font::load("demos/assets/fonts/Roboto/Roboto-Regular.ttf");
        auto render_glyphs = font.generate_render_glyphs("Hello World", 1.);
        auto text_shape = FontRenderGlyph::make_shape(render_glyphs);

        this->node_text_raw = new Node();
        this->node_text_raw->position = {0., 0.};
        this->node_text_raw->set_shape(text_shape);
        this->node_text_raw->set_sprite(font.baked_texture);

        this->root_node.add_child(this->node_text_raw);
    }

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            if (event.is_pressing(Keycode::q) or event.is_quit()) {
                get_engine()->quit();
                break;
            } else if (event.is_pressing(Keycode::w)) {
                this->camera.position += glm::dvec2(0., -0.5);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::a)) {
                this->camera.position += glm::dvec2(-0.5, 0.);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::s)) {
                this->camera.position += glm::dvec2(0., 0.5);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::d)) {
                this->camera.position += glm::dvec2(0.5, 0.);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::i)) {
                this->camera.scale += glm::dvec2(0.1, 0.1);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::o)) {
                this->camera.scale -= glm::dvec2(0.1, 0.1);
                this->camera.refresh();
            }
        }
    }
};


extern "C" int main(int argc, char *argv[])
{
    Engine eng;
    eng.create_window("kaacore fonts demo", 800, 600);
    DemoFontsScene scene;
    eng.run(&scene);

    return 0;
}
