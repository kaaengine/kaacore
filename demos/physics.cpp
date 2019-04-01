#include <vector>
#include <iostream>
#include <memory>
#include <random>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/scenes.h"
#include "kaacore/log.h"
#include "kaacore/input.h"
#include "kaacore/nodes.h"

using namespace kaacore;


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
        std::normal_distribution<double> speed_dist(0.0, 3.);

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
            ball->set_shape(Shape::Circle(0.3));
            ball->set_position(
                {position_dist(generator), position_dist(generator)}
            );
            ball->body.set_velocity(
                {speed_dist(generator), speed_dist(generator)}
            );

            Node* ball_hitbox = new Node(NodeType::hitbox);
            ball_hitbox->set_shape(Shape::Circle(0.3));
            ball_hitbox->hitbox.set_trigger_id(120);

            this->balls.push_back(ball);
            container->add_child(ball);
            ball->add_child(ball_hitbox);
        }

        this->container->space.set_collision_handler(120, 120,
            [](const Arbiter arbiter,
               const CollisionPair pair_a, const CollisionPair pair_b) -> uint8_t
            {
                std::cout << "Collision! " << int(arbiter.phase) << std::endl;
                pair_a.body_node->body.set_velocity({-0.8, -2.5});
                // pair_b.body_node->body.set_velocity({0.8, -2.5});
                delete pair_b.body_node;
                return 1;
            }, CollisionPhase::begin | CollisionPhase::separate
        );
        this->container->space.set_gravity({0.0, 2.5});
        this->box->body.set_angular_velocity(0.05);
    }

    void update(uint32_t dt) override
    {
        log<LogLevel::debug>("DemoScene update %lu/%llu", dt, this->time);
        auto texture = get_engine()->renderer->default_texture;

        for (auto const& event : this->get_events()) {
            if (event.is_pressing(Keycode::q) or event.is_quit()) {
                get_engine()->quit();
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
    eng.create_window("title", 800, 600);
    DemoScene scene;
    eng.run(&scene);

    return 0;
}
