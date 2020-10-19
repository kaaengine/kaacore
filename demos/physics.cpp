#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/engine.h"
#include "kaacore/input.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/physics.h"
#include "kaacore/scenes.h"

using namespace kaacore;

constexpr uint32_t mask_circle = 1 << 0;
constexpr uint32_t mask_polygon = 1 << 1;

struct DemoScene : Scene {
    double box_size = 4.;
    NodeOwnerPtr container;
    NodeOwnerPtr box;
    NodeOwnerPtr wall_l;
    NodeOwnerPtr wall_t;
    NodeOwnerPtr wall_r;
    NodeOwnerPtr wall_b;

    std::vector<NodePtr> balls;

    bool delete_on_collision = false;
    bool change_shape_on_collision = false;

    Shape query_shape;

    glm::dvec4 default_hitbox_color = {0., 0., 1., 0.5};
    glm::dvec4 queried_hitbox_color = {1., 0., 1., 0.7};

    NodeOwnerPtr init_wall(const glm::dvec2& a, const glm::dvec2& b)
    {
        NodeOwnerPtr wall_hitbox = make_node(NodeType::hitbox);
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

        this->query_shape = Shape::Circle(1.2);

        this->container = make_node(NodeType::space);
        this->root_node.add_child(this->container);

        this->box = make_node(NodeType::body);
        this->box->body.body_type(BodyNodeType::kinematic);

        this->wall_l =
            this->init_wall({-box_size, +box_size}, {-box_size, -box_size});
        this->wall_l->hitbox.surface_velocity({0., 1e12});
        this->wall_t =
            this->init_wall({-box_size, -box_size}, {+box_size, -box_size});
        this->wall_t->hitbox.surface_velocity({-1e12, 0.});
        this->wall_r =
            this->init_wall({+box_size, -box_size}, {+box_size, +box_size});
        this->wall_r->hitbox.surface_velocity({0., -1e12});
        this->wall_b =
            this->init_wall({+box_size, +box_size}, {-box_size, +box_size});
        this->wall_b->hitbox.surface_velocity({1e12, 0.});

        this->box->add_child(this->wall_l);
        this->box->add_child(this->wall_t);
        this->box->add_child(this->wall_r);
        this->box->add_child(this->wall_b);

        this->container->add_child(this->box);

        for (int i = 0; i < 10; i++) {
            NodeOwnerPtr ball = make_node(NodeType::body);
            ball->body.body_type(BodyNodeType::dynamic);

            Shape& chosen_shape =
                shape_dist(generator) ? polygon_shape : circle_shape;

            ball->shape(chosen_shape);
            ball->scale({1.5, 1.5});
            ball->position(
                {position_dist(generator), position_dist(generator)});
            ball->color({1., 1., 0., 1.});
            ball->body.moment(10.);

            NodeOwnerPtr ball_hitbox = make_node(NodeType::hitbox);
            ball_hitbox->shape(chosen_shape);
            ball_hitbox->scale({1.5, 1.5});
            ball_hitbox->hitbox.trigger_id(120);
            ball_hitbox->hitbox.elasticity(0.3);
            ball_hitbox->hitbox.friction(0.5);
            ball_hitbox->color(this->default_hitbox_color);

            if (chosen_shape == polygon_shape) {
                ball_hitbox->hitbox.mask(mask_polygon);
            } else {
                ball_hitbox->hitbox.mask(mask_circle);
            }

            this->balls.push_back(ball);
            container->add_child(ball);
            ball->add_child(ball_hitbox);
        }

        this->container->space.set_collision_handler(
            120, 120,
            [&, circle_shape, polygon_shape](
                const Arbiter arbiter, CollisionPair pair_a,
                CollisionPair pair_b) -> uint8_t {
                std::cout << "Collision! " << int(arbiter.phase) << std::endl;
                if (this->delete_on_collision) {
                    pair_a.body_node.destroy();
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
        this->box->body.angular_velocity(-0.10);
    }

    void update(uint32_t dt) override
    {
        log<LogLevel::debug>("DemoScene update %lu.", dt);
        auto texture = get_engine()->renderer->default_texture;

        for (auto const& event : this->get_events()) {
            auto keyboard_key = event.keyboard_key();
            if (keyboard_key and keyboard_key->is_key_down()) {
                if (keyboard_key->key() == Keycode::q) {
                    get_engine()->quit();
                    break;
                } else if (keyboard_key->key() == Keycode::w) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0., -0.1));
                } else if (keyboard_key->key() == Keycode::a) {
                    this->container->position(
                        this->container->position() + glm::dvec2(-0.1, 0.));
                } else if (keyboard_key->key() == Keycode::s) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0., 0.1));
                } else if (keyboard_key->key() == Keycode::d) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0.1, 0.));
                } else if (keyboard_key->key() == Keycode::r) {
                    this->box.destroy();
                } else if (keyboard_key->key() == Keycode::t) {
                    this->container.destroy();
                } else if (keyboard_key->key() == Keycode::x) {
                    if (not this->balls.empty()) {
                        this->balls.back().destroy();
                        this->balls.pop_back();
                    }
                } else if (keyboard_key->key() == Keycode::l) {
                    std::cout << "Setting objects lifetime" << std::endl;
                    if (not this->balls.empty()) {
                        for (const auto node : this->balls) {
                            node->lifetime(5000);
                        }
                        this->balls.clear();
                    }
                } else if (keyboard_key->key() == Keycode::num_1) {
                    std::cout << "Enabling delete_on_collision" << std::endl;
                    this->delete_on_collision = true;
                } else if (keyboard_key->key() == Keycode::num_2) {
                    std::cout << "Enabling change_shape_on_collision"
                              << std::endl;
                    this->change_shape_on_collision = true;
                }
            }
        }

        auto results = this->container->space.query_shape_overlaps(
            this->query_shape, {0., 0.}, collision_bitmask_all, mask_circle);
        for (auto& res : results) {
            res.hitbox_node->color(this->queried_hitbox_color);
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    Engine eng({20, 20});
    eng.window->size({800, 600});
    eng.window->center();
    DemoScene scene;
    scene.camera().position({0., 0.});
    eng.run(&scene);

    return 0;
}
