#include <iostream>
#include <memory>

#include <SDL.h>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/scene.h"
#include "kaacore/log.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"


struct DemoScene : Scene {
    Node* node1;
    Node* node2;

    DemoScene() {
        this->node1 = new Node();
        this->node1->position = {3., 3.};
        this->node1->rotation = 1.;
        this->node1->scale = {1., 3.,};
        this->node1->shape = Shape::Box({2., 1.});
        this->node1->recalculate_matrix();
        this->node1->recalculate_render_data();

        this->root_node.add_child(this->node1);

        this->node2 = new Node();
        this->node2->position = {-3., 3.};
        this->node2->rotation = 10.;
        this->node2->scale = {1., 1.,};
        this->node2->shape = Shape::Circle({0., 0.}, 1.5);
        this->node2->recalculate_matrix();
        this->node2->recalculate_render_data();

        this->root_node.add_child(this->node2);
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
            if (event.is_pressing(Keycode::q) or event.is_quit()) {
                get_engine()->attach_scene(nullptr);
                break;
            } else if (event.is_pressing(Keycode::w)) {
                this->camera.position += glm::dvec2(0., -0.05);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::a)) {
                this->camera.position += glm::dvec2(-0.05, 0.);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::s)) {
                this->camera.position += glm::dvec2(0., 0.05);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::d)) {
                this->camera.position += glm::dvec2(0.05, 0.);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::i)) {
                this->camera.scale += glm::dvec2(0.1, 0.1);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::o)) {
                this->camera.scale -= glm::dvec2(0.1, 0.1);
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::r)) {
                this->camera.rotation += 0.2;
                this->camera.refresh();
            } else if (event.is_pressing(Keycode::m)) {
                this->node1->rotation += 0.2;
                this->node1->position.x += 1.;
                log("Node position: %lf %lf", this->node1->position.x, this->node1->position.y);
            } else if (event.is_pressing(Keycode::n)) {
                this->root_node.position += glm::dvec2(-1., -2.);
                log("World position: %lf %lf", this->root_node.position.x, this->root_node.position.y);
            } else if (event.is_pressing(Keycode::c)) {
                this->camera.position = this->node1->get_absolute_position();
                this->camera.refresh();
                log("Camera position: %lf %lf", this->camera.position.x, this->camera.position.y);
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
