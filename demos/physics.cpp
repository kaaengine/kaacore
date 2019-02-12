#include <vector>
#include <iostream>
#include <memory>
#include <random>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/scene.h"
#include "kaacore/log.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"


struct DemoScene : Scene {
    double box_size = 4.;
    Node* container;
    Node* box;
    Node* wall_l;
    Node* wall_t;
    Node* wall_r;
    Node* wall_b;

    std::vector<Node*> balls;

    Node* init_wall(const glm::dvec2& a, const glm::dvec2& b)
    {
        Node* wall_hitbox = new Node(NodeType::hitbox);
        wall_hitbox->set_shape(Shape::Segment(a, b));
        return wall_hitbox;
    }

    DemoScene()
    {
        std::default_random_engine generator;
        std::normal_distribution<double> position_dist(0.0, 2.0);
        std::normal_distribution<double> speed_dist(0.0, 0.003);

        this->container = new Node(NodeType::space);
        this->root_node.add_child(this->container);

        this->box = new Node(NodeType::body);
        this->box->body.set_body_type(BodyNodeType::kinematic);

        this->wall_l = this->init_wall(
            {-box_size, +box_size}, {-box_size, -box_size}
        );
        this->wall_t = this->init_wall(
            {-box_size, -box_size}, {+box_size, -box_size}
        );
        this->wall_r = this->init_wall(
            {+box_size, -box_size}, {+box_size, +box_size}
        );
        this->wall_b = this->init_wall(
            {+box_size, +box_size}, {-box_size, +box_size}
        );

        this->box->add_child(this->wall_l);
        this->box->add_child(this->wall_t);
        this->box->add_child(this->wall_r);
        this->box->add_child(this->wall_b);

        this->container->add_child(this->box);

        for (int i = 0; i < 5; i++) {
            Node* ball = new Node(NodeType::body);
            ball->body.set_body_type(BodyNodeType::dynamic);
            ball->set_shape(Shape::Circle({0., 0.}, 0.3));
            ball->set_position(
                {position_dist(generator), position_dist(generator)}
            );
            ball->body.set_velocity(
                {speed_dist(generator), speed_dist(generator)}
            );

            Node* ball_hitbox = new Node(NodeType::hitbox);
            ball_hitbox->set_shape(Shape::Circle({0., 0.}, 0.3));

            this->balls.push_back(ball);
            container->add_child(ball);
            ball->add_child(ball_hitbox);
        }
    }

    void update(uint32_t dt) override
    {
        log<LogLevel::debug>("DemoScene update %lu/%llu", dt, this->time);
        auto texture = get_engine()->renderer->default_texture;

        for (auto const& event : this->get_events()) {
            if (event.is_pressing(Keycode::q) or event.is_quit()) {
                get_engine()->attach_scene(nullptr);
                break;
            } else if (event.is_pressing(Keycode::w)) {
                this->container->set_position(this->container->position + glm::dvec2(0., -0.1));
            } else if (event.is_pressing(Keycode::a)) {
                this->container->set_position(this->container->position + glm::dvec2(-0.1, 0.));
            } else if (event.is_pressing(Keycode::s)) {
                this->container->set_position(this->container->position + glm::dvec2(0., 0.1));
            } else if (event.is_pressing(Keycode::d)) {
                this->container->set_position(this->container->position + glm::dvec2(0.1, 0.));
            }
        }
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