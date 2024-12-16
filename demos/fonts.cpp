#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/fonts.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/unicode_buffer.h"

using namespace std::chrono_literals;
using namespace std::literals::string_view_literals;

static const auto txt_lorem_ipsum = kaacore::UnicodeView{
    u"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse "
    u"ultricies lacus massa. Phasellus tempus convallis ligula, et fermentum "
    u"mauris tincidunt a. Donec consequat felis sed massa suscipit "
    u"pellentesque. "
    u"Zażółć gęślą jaźń!"sv
};

struct DemoFontsScene : kaacore::Scene {
    kaacore::NodePtr background;
    kaacore::NodePtr node_text;

    DemoFontsScene()
    {
        auto background = kaacore::make_node();
        background->shape(kaacore::Shape::Box({700, 570}));
        background->color({0.5, 0.5, 0.5, 1.});
        background->z_index(-10);
        this->background = root_node.add_child(background);
        auto font = kaacore::Font::load(
            "demos/assets/fonts/Roboto/Roboto-Regular.ttf",
            kaacore::UnicodeView{U"ĄĆĘŁŃÓŚŹŻąćęłńóśźż"sv}
        );
        auto node_text = kaacore::make_node(kaacore::NodeType::text);
        node_text->position({200., 0.});
        node_text->text.content(txt_lorem_ipsum);
        node_text->text.font_size(24.);
        node_text->text.line_width(270.);
        node_text->text.font(font);
        node_text->text.first_line_indent(15.);
        node_text->color({0., 0., 0., 1.});
        this->node_text = this->root_node.add_child(node_text);

        this->node_text->transition(kaacore::make_node_transitions_parallel({
            kaacore::make_node_transitions_sequence(
                {kaacore::make_node_transition<kaacore::NodePositionTransition>(
                     glm::dvec2(200., 200.), 2.s
                 ),
                 kaacore::make_node_transition<kaacore::NodePositionTransition>(
                     glm::dvec2(0., 300.), 2.s
                 ),
                 kaacore::make_node_transition<kaacore::NodeScaleTransition>(
                     glm::dvec2(1.5, 1.5), 1.5s
                 ),
                 kaacore::make_node_transition<kaacore::NodePositionTransition>(
                     glm::dvec2(0., -0.), 2.s
                 )}
            ),
            kaacore::make_node_transition<kaacore::NodeColorTransition>(
                glm::dvec4(1., 1., 1., 1.), 10.s
            ),
        }));
    }

    void update(const kaacore::Duration dt) override
    {
        for (auto const& event : this->get_events()) {
            if (auto keyboard_key = event.keyboard_key()) {
                if (keyboard_key->is_key_down()) {
                    if (keyboard_key->key() == kaacore::Keycode::q) {
                        kaacore::get_engine()->quit();
                        break;
                    } else if (keyboard_key->key() == kaacore::Keycode::w) {
                        this->camera().position(
                            this->camera().position() + glm::dvec2(0., -2.5)
                        );
                    } else if (keyboard_key->key() == kaacore::Keycode::a) {
                        this->camera().position(
                            this->camera().position() + glm::dvec2(-2.5, 0.)
                        );
                    } else if (keyboard_key->key() == kaacore::Keycode::s) {
                        this->camera().position(
                            this->camera().position() + glm::dvec2(0., 2.5)
                        );
                    } else if (keyboard_key->key() == kaacore::Keycode::d) {
                        this->camera().position(
                            this->camera().position() + glm::dvec2(2.5, 0.)
                        );
                    } else if (keyboard_key->key() == kaacore::Keycode::i) {
                        this->camera().scale(
                            this->camera().scale() + glm::dvec2(0.1, 0.1)
                        );
                    } else if (keyboard_key->key() == kaacore::Keycode::o) {
                        this->camera().scale(
                            this->camera().scale() - glm::dvec2(0.1, 0.1)
                        );
                    } else if (keyboard_key->key() == kaacore::Keycode::l) {
                        std::u16string buf{std::get<std::u16string_view>(
                            this->node_text->text.content().string_view_variant(
                            )
                        )};
                        buf += u"x";
                        this->node_text->text.content(kaacore::UnicodeView{buf}
                        );
                    } else if (keyboard_key->key() == kaacore::Keycode::k) {
                        std::u16string buf{std::get<std::u16string_view>(
                            this->node_text->text.content().string_view_variant(
                            )
                        )};
                        buf += u" ";
                        this->node_text->text.content(kaacore::UnicodeView{buf}
                        );
                    }
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng({800, 600});
    DemoFontsScene scene;
    scene.camera().position({0., 0.});
    eng.run(&scene);

    return 0;
}
