#pragma once

#include <glm/glm.hpp>
#include <chipmunk/chipmunk.h>


struct SpaceNode {
    cpSpace* cp_space = nullptr;
    uint32_t time_acc = 0;

    void initialize();
    void destroy();

    void simulate(uint32_t dt);
};


enum struct BodyNodeType {
    dynamic = cpBodyType::CP_BODY_TYPE_DYNAMIC,
    kinematic = cpBodyType::CP_BODY_TYPE_KINEMATIC,
    static_ = cpBodyType::CP_BODY_TYPE_STATIC
};


struct BodyNode {
    cpBody* cp_body = nullptr;

    void initialize();
    void destroy();
    void attach_to_simulation();

    void set_body_type(const BodyNodeType type);
    BodyNodeType get_body_type() const;

    void set_mass(const double m);
    double get_mass() const;

    void set_moment(const double i);
    double get_moment() const;

    void set_velocity(const glm::dvec2 velocity);
    glm::dvec2 get_velocity() const;

    void override_simulation_position();
    void sync_simulation_position();
};


struct HitboxNode {
    cpShape* cp_shape = nullptr;

    void initialize();
    void destroy();
    void update_physics_shape();
    void attach_to_simulation();
};
