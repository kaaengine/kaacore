#include <cstdlib>
#include <iostream>

#include "kaacore/clock.h"
#include "kaacore/engine.h"
#include "kaacore/log.h"
#include "kaacore/node_transitions.h"
#include "kaacore/nodes.h"
#include "kaacore/resources.h"
#include "kaacore/scenes.h"
#include "kaacore/textures.h"
#include "kaacore/transitions.h"

using std::atoi;
using namespace std::chrono_literals;

struct SpritesDemoScene : kaacore::Scene {
    kaacore::NodePtr animating_node;
    kaacore::ResourceReference<kaacore::Texture> texture;

    SpritesDemoScene(
        const char* filepath, int frame_w, int frame_h, int padding_x,
        int padding_y)
    {
        this->texture = kaacore::ImageTexture::load(filepath);
        kaacore::Sprite sprite{this->texture};
        auto frames = kaacore::split_spritesheet(
            sprite, {frame_w, frame_h}, 0, 0, {padding_x, padding_y});

        auto animating_node = kaacore::make_node();
        animating_node->shape(kaacore::Shape::Box({3, 3}));
        animating_node->transition(
            kaacore::make_node_transition<kaacore::NodeSpriteTransition>(
                frames, 5.s, kaacore::TransitionWarping(0, true)));
        this->animating_node = this->root_node.add_child(animating_node);
    }

    void update(const kaacore::Duration dt) override
    {
        KAACORE_APP_LOG_DEBUG("DemoScene update {}s.", dt.count());

        for (auto const& event : this->get_events()) {
            if (auto keyboard_key = event.keyboard_key()) {
                if (keyboard_key->key() == kaacore::Keycode::q) {
                    kaacore::get_engine()->quit();
                    break;
                } else if (keyboard_key->key() == kaacore::Keycode::w) {
                    this->animating_node->position(
                        this->animating_node->position() +
                        glm::dvec2(0., -0.1));
                } else if (keyboard_key->key() == kaacore::Keycode::a) {
                    this->animating_node->position(
                        this->animating_node->position() +
                        glm::dvec2(-0.1, 0.));
                } else if (keyboard_key->key() == kaacore::Keycode::s) {
                    this->animating_node->position(
                        this->animating_node->position() + glm::dvec2(0., 0.1));
                } else if (keyboard_key->key() == kaacore::Keycode::d) {
                    this->animating_node->position(
                        this->animating_node->position() + glm::dvec2(0.1, 0.));
                }
            }
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    if (argc != 6) {
        std::cout
            << "Usage: <image_path> <frame_w> <frame_h> <padding_x> <padding_y>"
            << std::endl;
        return 1;
    }

    kaacore::Engine eng({5, 5});
    eng.window->size({800, 600});
    eng.window->center();
    SpritesDemoScene scene{argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                           atoi(argv[5])};
    scene.camera().position({0., 0.});
    eng.run(&scene);

    return 0;
}
