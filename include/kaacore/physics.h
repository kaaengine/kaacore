#pragma once

#include <functional>

#include <glm/glm.hpp>
#include <chipmunk/chipmunk.h>


typedef size_t CollisionTriggerId;

struct Node;
struct BodyNode;
struct HitboxNode;


struct Arbiter {
    cpArbiter* cp_arbiter;

    Arbiter(cpArbiter* arbiter);
};


enum struct CollisionPhase {
    begin = 1,
    pre_solve = 2,
    post_solve = 4,
    separate = 8,
    any_phase = 15
};


uint8_t operator|(CollisionPhase phase, uint8_t other);
uint8_t operator|(CollisionPhase phase, CollisionPhase other);
uint8_t operator&(CollisionPhase phase, uint8_t other);
uint8_t operator&(CollisionPhase phase, CollisionPhase other);


struct CollisionPair {
    Node* body;
    Node* hitbox;

    CollisionPair(BodyNode* body, HitboxNode* hitbox);
};


typedef std::function<uint8_t(CollisionPhase, Arbiter, CollisionPair, CollisionPair)> \
        CollisionHandlerFunc;


struct SpaceNode {
    cpSpace* cp_space = nullptr;
    uint32_t time_acc = 0;

    void initialize();
    void destroy();

    void simulate(uint32_t dt);
    void set_collision_handler(
        CollisionTriggerId trigger_a, CollisionTriggerId trigger_b,
        CollisionHandlerFunc handler,
        uint8_t phases_mask=uint8_t(CollisionPhase::any_phase),
        bool only_non_deleted_nodes=true
    );
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

    void set_trigger_id(CollisionTriggerId trigger_id);
    CollisionTriggerId get_trigger_id();
};
