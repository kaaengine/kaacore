#include <algorithm>
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

using namespace std::chrono_literals;

constexpr uint32_t mask_circle = 1 << 0;
constexpr uint32_t mask_polygon = 1 << 1;

struct DemoScene : kaacore::Scene {
    double box_size = 4.;
    kaacore::NodePtr container;
    kaacore::NodePtr box;
    kaacore::NodePtr wall_l;
    kaacore::NodePtr wall_t;
    kaacore::NodePtr wall_r;
    kaacore::NodePtr wall_b;
    bool time_scaled = false;

    std::vector<kaacore::NodePtr> balls;

    bool delete_on_collision = false;
    bool change_shape_on_collision = false;

    kaacore::Shape query_shape;

    glm::dvec4 default_hitbox_color = {0., 0., 1., 0.5};
    glm::dvec4 queried_hitbox_color = {1., 0., 1., 0.7};

    kaacore::NodeOwnerPtr init_wall(const glm::dvec2& a, const glm::dvec2& b)
    {
        kaacore::NodeOwnerPtr wall_hitbox =
            make_node(kaacore::NodeType::hitbox);
        wall_hitbox->shape(kaacore::Shape::Segment(a, b));
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
        kaacore::Shape polygon_shape =
            kaacore::Shape::Polygon({{0.3, 0}, {0, 0.3}, {-0.3, 0}, {0, -0.7}});
        kaacore::Shape circle_shape = kaacore::Shape::Circle(0.3);

        this->query_shape = kaacore::Shape::Circle(1.2);

        auto container = kaacore::make_node(kaacore::NodeType::space);
        this->container = this->root_node.add_child(container);

        auto box = make_node(kaacore::NodeType::body);
        box->body.body_type(kaacore::BodyNodeType::kinematic);

        auto wall_l =
            this->init_wall({-box_size, +box_size}, {-box_size, -box_size});
        wall_l->hitbox.surface_velocity({0., 1e12});
        auto wall_t =
            this->init_wall({-box_size, -box_size}, {+box_size, -box_size});
        wall_t->hitbox.surface_velocity({-1e12, 0.});
        auto wall_r =
            this->init_wall({+box_size, -box_size}, {+box_size, +box_size});
        wall_r->hitbox.surface_velocity({0., -1e12});
        auto wall_b =
            this->init_wall({+box_size, +box_size}, {-box_size, +box_size});
        wall_b->hitbox.surface_velocity({1e12, 0.});

        this->wall_l = box->add_child(wall_l);
        this->wall_t = box->add_child(wall_t);
        this->wall_r = box->add_child(wall_r);
        this->wall_b = box->add_child(wall_b);

        this->box = this->container->add_child(box);

        for (int i = 0; i < 10; i++) {
            kaacore::NodeOwnerPtr ball = make_node(kaacore::NodeType::body);
            ball->body.body_type(kaacore::BodyNodeType::dynamic);

            kaacore::Shape& chosen_shape =
                shape_dist(generator) ? polygon_shape : circle_shape;

            ball->shape(chosen_shape);
            ball->scale({1.5, 1.5});
            ball->position(
                {position_dist(generator), position_dist(generator)});
            ball->color({1., 1., 0., 1.});
            ball->body.moment(10.);

            kaacore::NodeOwnerPtr ball_hitbox =
                kaacore::make_node(kaacore::NodeType::hitbox);
            ball_hitbox->shape(chosen_shape);
            ball_hitbox->scale({1.5, 1.5});
            ball_hitbox->hitbox.trigger_id(120);
            ball_hitbox->hitbox.elasticity(0.9);
            ball_hitbox->hitbox.friction(0.5);
            ball_hitbox->color(this->default_hitbox_color);

            if (chosen_shape == polygon_shape) {
                ball_hitbox->hitbox.mask(mask_polygon);
            } else {
                ball_hitbox->hitbox.mask(mask_circle);
            }

            ball->add_child(ball_hitbox);
            this->balls.push_back(this->container->add_child(ball));
        }

        this->container->space.set_collision_handler(
            120, 120,
            [&, circle_shape, polygon_shape](
                const kaacore::Arbiter arbiter, kaacore::CollisionPair pair_a,
                kaacore::CollisionPair pair_b) -> uint8_t {
                std::cout << "Collision! " << int(arbiter.phase) << std::endl;
                if (this->delete_on_collision) {
                    pair_a.body_node.destroy();
                } else if (
                    arbiter.phase == kaacore::CollisionPhase::separate and
                    this->change_shape_on_collision) {
                    if (pair_a.hitbox_node->shape().type ==
                        kaacore::ShapeType::circle) {
                        pair_a.body_node->shape(polygon_shape);
                        pair_a.hitbox_node->shape(polygon_shape);
                    } else {
                        pair_a.body_node->shape(circle_shape);
                        pair_a.hitbox_node->shape(circle_shape);
                    }
                    if (pair_b.hitbox_node->shape().type ==
                        kaacore::ShapeType::circle) {
                        pair_b.body_node->shape(polygon_shape);
                        pair_b.hitbox_node->shape(polygon_shape);
                    } else {
                        pair_b.body_node->shape(circle_shape);
                        pair_b.hitbox_node->shape(circle_shape);
                    }
                }
                return 1;
            },
            kaacore::CollisionPhase::begin | kaacore::CollisionPhase::separate);
        this->container->space.gravity({0.0, 2.5});
        this->box->body.angular_velocity(-0.10);
    }

    void update(const kaacore::Duration dt) override
    {
        KAACORE_APP_LOG_DEBUG("DemoScene update, dt: {}s.", dt.count());
        auto texture = kaacore::get_engine()->renderer->default_texture;

        for (auto const& event : this->get_events()) {
            if (auto keyboard_key = event.keyboard_key();
                keyboard_key and keyboard_key->is_key_down()) {
                if (keyboard_key->key() == kaacore::Keycode::q) {
                    kaacore::get_engine()->quit();
                    break;
                } else if (keyboard_key->key() == kaacore::Keycode::w) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0., -0.1));
                } else if (keyboard_key->key() == kaacore::Keycode::a) {
                    this->container->position(
                        this->container->position() + glm::dvec2(-0.1, 0.));
                } else if (keyboard_key->key() == kaacore::Keycode::t) {
                    if (this->time_scaled) {
                        this->time_scale(1.);
                    } else {
                        this->time_scale(0.25);
                    }
                    this->time_scaled = not this->time_scaled;
                } else if (keyboard_key->key() == kaacore::Keycode::s) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0., 0.1));
                } else if (keyboard_key->key() == kaacore::Keycode::d) {
                    this->container->position(
                        this->container->position() + glm::dvec2(0.1, 0.));
                } else if (keyboard_key->key() == kaacore::Keycode::r) {
                    this->box.destroy();
                } else if (keyboard_key->key() == kaacore::Keycode::t) {
                    for (auto child : this->container->children()) {
                        kaacore::NodePtr(child).destroy();
                    }
                } else if (keyboard_key->key() == kaacore::Keycode::x) {
                    if (not this->balls.empty()) {
                        this->balls.back().destroy();
                        this->balls.pop_back();
                    }
                } else if (keyboard_key->key() == kaacore::Keycode::l) {
                    std::cout << "Setting objects lifetime" << std::endl;
                    if (not this->balls.empty()) {
                        for (const auto node : this->balls) {
                            node->lifetime(5.s);
                        }
                        this->balls.clear();
                    }
                } else if (keyboard_key->key() == kaacore::Keycode::num_1) {
                    std::cout << "Enabling delete_on_collision" << std::endl;
                    this->delete_on_collision = true;
                } else if (keyboard_key->key() == kaacore::Keycode::num_2) {
                    std::cout << "Enabling change_shape_on_collision"
                              << std::endl;
                    this->change_shape_on_collision = true;
                }
            } else if (auto mouse_button = event.mouse_button();
                       mouse_button and mouse_button->is_button_down()) {
                if (mouse_button->button() == kaacore::MouseButton::left) {
                    auto point_results =
                        this->container->space.query_point_neighbors(
                            this->camera().unproject_position(
                                mouse_button->position()),
                            10.);
                    if (not point_results.empty()) {
                        auto nearest_neighbor = *std::min_element(
                            point_results.begin(), point_results.end(),
                            [](const auto a, const auto b) {
                                return a.distance < b.distance;
                            });
                        auto hit_indicator = kaacore::make_node();
                        hit_indicator->position(nearest_neighbor.point);
                        hit_indicator->shape(kaacore::Shape::Circle(0.1));
                        hit_indicator->color(glm::dvec4(0., 1., 0., 0.4));
                        hit_indicator->lifetime(1.s);
                        this->container->add_child(hit_indicator);
                    }
                }
            }
        }

        auto results = this->container->space.query_shape_overlaps(
            this->query_shape, kaacore::collision_bitmask_all, mask_circle);
        for (auto& res : results) {
            res.hitbox_node->color(this->queried_hitbox_color);
        }

        auto raycast_results = this->container->space.query_ray(
            glm::dvec2{-10, 0}, glm::dvec2{10, 0});
        for (auto& res : raycast_results) {
            auto hit_indicator = kaacore::make_node();
            hit_indicator->position(res.point);
            hit_indicator->shape(kaacore::Shape::Circle(0.1));
            hit_indicator->color(glm::dvec4(1., 0., 0., 0.4));
            hit_indicator->lifetime(0.09s);

            auto hit_indicator_normal = kaacore::make_node();
            hit_indicator_normal->position(res.normal);
            hit_indicator_normal->shape(kaacore::Shape::Circle(0.1));
            hit_indicator_normal->color(glm::dvec4(1., 1., 0., 0.4));
            hit_indicator_normal->lifetime(0.09s);
            hit_indicator->add_child(hit_indicator_normal);

            this->container->add_child(hit_indicator);
        }
    }
};

extern "C" int
main(int argc, char* argv[])
{
    kaacore::Engine eng({20, 20});
    kaacore::set_logging_level("physics", spdlog::level::debug);
    eng.window->size({800, 600});
    eng.window->center();
    DemoScene scene;
    scene.camera().position({0., 0.});
    eng.run(&scene);

    return 0;
}
