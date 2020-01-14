#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/scenes.h"

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

    bool delete_on_collision = false;
    bool change_shape_on_collision = false;

    Shape query_shape;

    glm::dvec4 default_hitbox_color = {0., 0., 1., 0.5};
    glm::dvec4 queried_hitbox_color = {1., 0., 1., 0.7};

    Node* init_wall(const glm::dvec2& a, const glm::dvec2& b)
    {
        Node* wall_hitbox = new Node(NodeType::hitbox);
        wall_hitbox->shape(Shape::Segment(a, b));
        wall_hitbox->color({1., 0.0, 0.6, 0.4});
        wall_hitbox->scale({1.5, 1.5});
        return wall_hitbox;
    }

    DemoScene()
    {
        std::random_device random_dev;
        std::default_random_engine generator(random_dev());
        std::normal_distribution<double> position_dist(0.0, 1.5);
        std::normal_distribution<double> speed_dist(0.0, 3.);
        std::uniform_int_distribution<> shape_dist(0, 1);
        Shape polygon_shape =
            Shape::Polygon({{0.3, 0}, {0, 0.3}, {-0.3, 0}, {0, -0.7}});
        Shape circle_shape = Shape::Circle(0.3);

        this->query_shape = Shape::Circle(1.5);

        this->container = new Node(NodeType::space);
        this->root_node.add_child(this->container);

        this->box = new Node(NodeType::body);
        this->box->body.body_type(BodyNodeType::kinematic);

        this->wall_l =
            this->init_wall({-box_size, +box_size}, {-box_size, -box_size});
        this->wall_t =
            this->init_wall({-box_size, -box_size}, {+box_size, -box_size});
        this->wall_r =
            this->init_wall({+box_size, -box_size}, {+box_size, +box_size});
        this->wall_b =
            this->init_wall({+box_size, +box_size}, {-box_size, +box_size});

        this->box->add_child(this->wall_l);
        this->box->add_child(this->wall_t);
        this->box->add_child(this->wall_r);
        this->box->add_child(this->wall_b);

        this->container->add_child(this->box);

        for (int i = 0; i < 10; i++) {
            Node* ball = new Node(NodeType::body);
            ball->body.body_type(BodyNodeType::dynamic);

            Shape& chosen_shape =
                shape_dist(generator) ? polygon_shape : circle_shape;

            ball->shape(chosen_shape);
            ball->scale({1.5, 1.5});
            ball->position(
                {position_dist(generator), position_dist(generator)});
            ball->color({1., 1., 0., 1.});
            ball->body.moment(10.);

            Node* ball_hitbox = new Node(NodeType::hitbox);
            ball_hitbox->shape(chosen_shape);
            ball_hitbox->scale({1.5, 1.5});
            ball_hitbox->hitbox.trigger_id(120);
            ball_hitbox->color(this->default_hitbox_color);
            ball_hitbox->z_index(ball->z_index() + 1);

            this->balls.push_back(ball);
            container->add_child(ball);
            ball->add_child(ball_hitbox);
        }

        this->container->space.set_collision_handler(
            120, 120,
            [&, circle_shape, polygon_shape](
                const Arbiter arbiter, const CollisionPair pair_a,
                const CollisionPair pair_b) -> uint8_t {
                std::cout << "Collision! " << int(arbiter.phase) << std::endl;
                if (this->delete_on_collision) {
                    delete pair_a.body_node;
                } else if (
                    arbiter.phase == CollisionPhase::separate and
                    this->change_shape_on_collision) {
                    if (pair_a.hitbox_node->shape().type == ShapeType::circle) {
                        pair_a.body_node->shape(polygon_shape);
                        pair_a.hitbox_node->shape(polygon_shape);
                    } else {
                        pair_a.body_node->shape(circle_shape);
                        pair_a.hitbox_node->shape(circle_shape);
                    }
                    if (pair_b.hitbox_node->shape().type == ShapeType::circle) {
                        pair_b.body_node->shape(polygon_shape);
                        pair_b.hitbox_node->shape(polygon_shape);
                    } else {
                        pair_b.body_node->shape(circle_shape);
                        pair_b.hitbox_node->shape(circle_shape);
                    }
                }
                return 1;
            },
            CollisionPhase::begin | CollisionPhase::separate);
        this->container->space.gravity({0.0, 2.5});
        this->box->body.angular_velocity(-0.50);
    }

    void update(uint32_t dt) override
    {
        log<LogLevel::debug>("DemoScene update %lu.", dt);
        auto texture = get_engine()->renderer->default_texture;

        for (auto const& event : this->get_events()) {
            auto system = event.system();
            if (system and system->quit()) {
                get_engine()->quit();
                break;
            }

            if (auto keyboard = event.keyboard()) {
                if (keyboard->is_pressing(Keycode::q)) {
                    get_engine()->quit();
                    break;
                } else if (keyboard->is_pressing(Keycode::w)) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0., -0.1));
                } else if (keyboard->is_pressing(Keycode::a)) {
                    this->container->position(
                        this->container->position() + glm::dvec2(-0.1, 0.));
                } else if (keyboard->is_pressing(Keycode::s)) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0., 0.1));
                } else if (keyboard->is_pressing(Keycode::d)) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0.1, 0.));
                } else if (keyboard->is_pressing(Keycode::r)) {
                    delete this->box;
                } else if (keyboard->is_pressing(Keycode::t)) {
                    delete this->container;
                } else if (keyboard->is_pressing(Keycode::x)) {
                    if (not this->balls.empty()) {
                        delete this->balls.back();
                        this->balls.pop_back();
                    }
                } else if (keyboard->is_pressing(Keycode::l)) {
                    std::cout << "Setting objects lifetime" << std::endl;
                    for (const auto node : this->balls) {
                        node->lifetime(5000);
                    }
                } else if (keyboard->is_pressing(Keycode::num_1)) {
                    std::cout << "Enabling delete_on_collision" << std::endl;
                    this->delete_on_collision = true;
                } else if (keyboard->is_pressing(Keycode::num_2)) {
                    std::cout << "Enabling change_shape_on_collision"
                              << std::endl;
                    this->change_shape_on_collision = true;
                }
            }
        }

        auto results =
            this->container->space.query_shape_overlaps(this->query_shape);
        for (auto& res : results) {
            res.hitbox_node->color(this->queried_hitbox_color);
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({15, 15});
    eng.window->show();
    DemoScene scene;
    scene.camera.position = {0., 0.};
    eng.run(&scene);

    return 0;
}
