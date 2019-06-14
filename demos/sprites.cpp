#include <iostream>
#include <cstdlib>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"
#include "kaacore/resources.h"
#include "kaacore/texture_loader.h"
#include "kaacore/nodes.h"
#include "kaacore/log.h"

using namespace kaacore;

using std::atoi;


struct SpritesDemoScene : Scene {
    Node* animating_node;
    Resource<Image> image_file;

    SpritesDemoScene(const char* filepath, int crop_x, int crop_y, int frame_w, int frame_h)
    {
        this->image_file = Image::load(filepath);
        Sprite sprite{this->image_file};
        sprite.dimensions = {crop_x, crop_y};
        sprite.frame_dimensions = {frame_w, frame_h};
        sprite.animation_frame_duration = 30;
        sprite.animation_loop = true;

        this->animating_node = new Node();
        this->animating_node->set_shape(Shape::Box({3, 3}));
        this->animating_node->set_sprite(sprite);
        this->root_node.add_child(this->animating_node);
    }

    void update(uint32_t dt) override
    {
        log<LogLevel::debug>("DemoScene update %lu/%llu", dt, this->time);

        for (auto const& event : this->get_events()) {
            if (event.is_pressing(Keycode::q) or event.is_quit()) {
                get_engine()->quit();
                break;
            } else if (event.is_pressing(Keycode::w)) {
                this->animating_node->set_position(this->animating_node->position + glm::dvec2(0., -0.1));
            } else if (event.is_pressing(Keycode::a)) {
                this->animating_node->set_position(this->animating_node->position + glm::dvec2(-0.1, 0.));
            } else if (event.is_pressing(Keycode::s)) {
                this->animating_node->set_position(this->animating_node->position + glm::dvec2(0., 0.1));
            } else if (event.is_pressing(Keycode::d)) {
                this->animating_node->set_position(this->animating_node->position + glm::dvec2(0.1, 0.));
            }
        }
    }
};

extern "C" int main(int argc, char *argv[])
{
    if (argc != 6) {
        std::cout << "Usage: <image_path> <crop_x> <crop_y> <frame_w> <frame_h>" << std::endl;
        return 1;
    }

    Engine eng({5, 5});
    eng.window->show();
    SpritesDemoScene scene{argv[1], atoi(argv[2]), atoi(argv[3]),
                                    atoi(argv[4]), atoi(argv[5])};
    eng.run(&scene);

    return 0;
}
