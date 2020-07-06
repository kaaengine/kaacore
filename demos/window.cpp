#include <cstdlib>
#include <iostream>
#include <vector>

#include "kaacore/engine.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"
#include "kaacore/transitions.h"

using namespace kaacore;

struct WindowDemoScene : Scene {
    WindowDemoScene()
    {
        this->camera().position({0., 0.});

        auto circle_node = make_node();
        circle_node->shape(Shape::Circle(49.));
        circle_node->transition(make_node_transition<NodeColorTransition>(
            glm::dvec4{1., 0.3, 0.3, 0.7}, 1000, TransitionWarping{0, true}));
        this->root_node.add_child(circle_node);
    }

    void update(uint32_t dt) override
    {
        for (auto const& event : this->get_events()) {
            if (event.keyboard_key() and event.keyboard_key()->is_key_down()) {
                auto key = event.keyboard_key()->key();
                if (key == Keycode::q) {
                    get_engine()->quit();
                    break;
                } else if (key == Keycode::w) {
                    get_engine()->window->position(
                        get_engine()->window->position() - glm::uvec2{0, 1});
                } else if (key == Keycode::s) {
                    get_engine()->window->position(
                        get_engine()->window->position() + glm::uvec2{0, 1});
                } else if (key == Keycode::a) {
                    get_engine()->window->position(
                        get_engine()->window->position() - glm::uvec2{1, 0});
                } else if (key == Keycode::d) {
                    get_engine()->window->position(
                        get_engine()->window->position() + glm::uvec2{1, 0});
                } else if (key == Keycode::r) {
                    get_engine()->window->size(
                        get_engine()->window->size() + glm::uvec2{5, 5});
                } else if (key == Keycode::t) {
                    get_engine()->window->size(
                        get_engine()->window->size() - glm::uvec2{5, 5});
                } else if (key == Keycode::z) {
                    get_engine()->window->minimize();
                } else if (key == Keycode::x) {
                    get_engine()->window->maximize();
                } else if (key == Keycode::c) {
                    get_engine()->window->restore();
                } else if (key == Keycode::f) {
                    get_engine()->window->fullscreen(
                        not get_engine()->window->fullscreen());
                } else if (key == Keycode::p) {
                    std::cout
                        << "Clipboard: "
                        << get_engine()
                               ->input_manager->system.get_clipboard_text()
                        << "\n";
                } else if (key == Keycode::o) {
                    get_engine()->input_manager->system.set_clipboard_text(
                        "KAA TEXT!");
                }
            } else if (event.window() and event.window()->is_moved()) {
                std::cout << "EVENT: Window moved\n";
            } else if (event.window() and event.window()->is_resized()) {
                std::cout << "EVENT: Window resized\n";
            } else if (
                event.system() and event.system()->is_clipboard_updated()) {
                std::cout << "EVENT: Clipboard updated\n";
            }
        }

        if (get_engine()->input_manager->keyboard.is_pressed(Keycode::space)) {
            std::cout << "STATE: SPACE is pressed\n";
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({100, 100});
    eng.window->size({800, 600});
    eng.window->center();
    WindowDemoScene scene;
    eng.run(&scene);

    return 0;
}
