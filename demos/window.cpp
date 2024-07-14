#include <cstdlib>
#include <iostream>
#include <vector>

#include "kaacore/engine.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/transitions.h"

using namespace std::chrono_literals;

struct WindowDemoScene : kaacore::Scene {
    WindowDemoScene()
    {
        this->camera().position({0., 0.});

        auto circle_node = kaacore::make_node();
        circle_node->shape(kaacore::Shape::Circle(49.));
        circle_node->transition(
            kaacore::make_node_transition<kaacore::NodeColorTransition>(
                glm::dvec4{1., 0.3, 0.3, 0.7}, 1.s,
                kaacore::TransitionWarping{0, true}
            )
        );
        this->root_node.add_child(circle_node);
    }

    void update(const kaacore::Duration dt) override
    {
        for (auto const& event : this->get_events()) {
            if (event.keyboard_key() and event.keyboard_key()->is_key_down()) {
                auto key = event.keyboard_key()->key();
                if (key == kaacore::Keycode::q) {
                    kaacore::get_engine()->quit();
                    break;
                } else if (key == kaacore::Keycode::w) {
                    kaacore::get_engine()->window->position(
                        kaacore::get_engine()->window->position() -
                        glm::uvec2{0, 1}
                    );
                } else if (key == kaacore::Keycode::s) {
                    kaacore::get_engine()->window->position(
                        kaacore::get_engine()->window->position() +
                        glm::uvec2{0, 1}
                    );
                } else if (key == kaacore::Keycode::a) {
                    kaacore::get_engine()->window->position(
                        kaacore::get_engine()->window->position() -
                        glm::uvec2{1, 0}
                    );
                } else if (key == kaacore::Keycode::d) {
                    kaacore::get_engine()->window->position(
                        kaacore::get_engine()->window->position() +
                        glm::uvec2{1, 0}
                    );
                } else if (key == kaacore::Keycode::r) {
                    kaacore::get_engine()->window->size(
                        kaacore::get_engine()->window->size() + glm::uvec2{5, 5}
                    );
                } else if (key == kaacore::Keycode::t) {
                    kaacore::get_engine()->window->size(
                        kaacore::get_engine()->window->size() - glm::uvec2{5, 5}
                    );
                } else if (key == kaacore::Keycode::z) {
                    kaacore::get_engine()->window->minimize();
                } else if (key == kaacore::Keycode::x) {
                    kaacore::get_engine()->window->maximize();
                } else if (key == kaacore::Keycode::c) {
                    kaacore::get_engine()->window->restore();
                } else if (key == kaacore::Keycode::f) {
                    kaacore::get_engine()->window->fullscreen(
                        not kaacore::get_engine()->window->fullscreen()
                    );
                } else if (key == kaacore::Keycode::p) {
                    std::cout
                        << "Clipboard: "
                        << kaacore::get_engine()
                               ->input_manager->system.get_clipboard_text()
                        << "\n";
                } else if (key == kaacore::Keycode::o) {
                    kaacore::get_engine()
                        ->input_manager->system.set_clipboard_text("KAA TEXT!");
                }
            } else if (event.window() and event.window()->is_moved()) {
                std::cout << "EVENT: Window moved\n";
            } else if (event.window() and event.window()->is_resized()) {
                std::cout << "EVENT: Window resized\n";
            } else if (event.system() and
                       event.system()->is_clipboard_updated()) {
                std::cout << "EVENT: Clipboard updated\n";
            }
        }

        if (kaacore::get_engine()->input_manager->keyboard.is_pressed(
                kaacore::Keycode::space
            )) {
            std::cout << "STATE: SPACE is pressed\n";
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng({100, 100});
    eng.window->size({800, 600});
    eng.window->center();
    WindowDemoScene scene;
    eng.run(&scene);

    return 0;
}
