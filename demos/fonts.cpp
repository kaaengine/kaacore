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


static const std::string txt_lorem_ipsum = \
     "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse " \
     "ultricies lacus massa. Phasellus tempus convallis ligula, et fermentum " \
     "mauris tincidunt a. Donec consequat felis sed massa suscipit " \
     "pellentesque. Etiam ullamcorper lacinia arcu ut vehicula. Morbi mattis " \
     "lacus velit, nec tincidunt diam vulputate sit amet. Maecenas fermentum " \
     "sagittis justo, id lacinia justo auctor ut. Maecenas mollis neque sit " \
     "amet tortor porttitor lobortis.";


struct DemoFontsScene : Scene {
    Node* background;
    Node* node_text_raw;
    Node* node_text;

    DemoFontsScene() {
        this->camera.size = {800., 600.};
        this->camera.refresh();

        auto font = Font::load("demos/assets/fonts/Roboto/Roboto-Regular.ttf");
        auto render_glyphs = font->generate_render_glyphs("Hello World \n\n\nFooBar FooBar", 30.);
        FontRenderGlyph::arrange_glyphs(render_glyphs, 15., 35., 120.);
        auto text_shape = FontRenderGlyph::make_shape(render_glyphs);

        this->background = new Node();
        this->background->set_shape(Shape::Box({700, 570}));
        this->background->color = {0.5, 0.5, 0.5, 1.};
        this->background->z_index = -10;
        this->root_node.add_child(this->background);

        this->node_text_raw = new Node();
        this->node_text_raw->position = {-250., 0.};
        this->node_text_raw->set_shape(text_shape);
        this->node_text_raw->set_sprite(font->baked_texture);
        this->root_node.add_child(this->node_text_raw);

        this->node_text = new Node(NodeType::text);
        this->node_text->position = {0., -200.};
        this->node_text->text.font(font);
        this->node_text->text.content(txt_lorem_ipsum);
        this->node_text->text.font_size(24.);
        this->node_text->text.line_width(270.);
        this->node_text->text.first_line_indent(15.);
        this->node_text->color = {0., 0., 0., 1.};
        this->root_node.add_child(this->node_text);
    }

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            if (event.is_pressing(Keycode::q) or event.is_quit()) {
                get_engine()->quit();
                break;
            } else if (event.is_pressing(Keycode::w)) {
                this->camera.position += glm::dvec2(0., -2.5);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::a)) {
                this->camera.position += glm::dvec2(-2.5, 0.);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::s)) {
                this->camera.position += glm::dvec2(0., 2.5);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::d)) {
                this->camera.position += glm::dvec2(2.5, 0.);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::i)) {
                this->camera.scale += glm::dvec2(0.1, 0.1);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::o)) {
                this->camera.scale -= glm::dvec2(0.1, 0.1);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::l)) {
                this->node_text->text.content(
                    this->node_text->text.content() + "x"
                );
            } else if (event.is_pressing(Keycode::k)) {
                this->node_text->text.content(
                    this->node_text->text.content() + " "
                );
            }
        }
    }
};


extern "C" int main(int argc, char *argv[])
{
    Engine eng;
    DemoFontsScene scene;
    eng.window->show();
    eng.run(&scene);

    return 0;
}
