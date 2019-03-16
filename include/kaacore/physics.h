#pragma once

#include <functional>

#include <glm/glm.hpp>
#include <chipmunk/chipmunk.h>


namespace kaacore {

typedef size_t CollisionTriggerId;
typedef size_t CollisionGroup;
typedef cpBitmask CollisionBitmask;

constexpr uint32_t default_simulation_step_size = 10;

struct Node;
struct SpaceNode;
struct BodyNode;
struct HitboxNode;


enum struct CollisionPhase {
    begin = 1,
    pre_solve = 2,
    post_solve = 4,
    separate = 8,
    any_phase = 15
};


struct Arbiter {
    cpArbiter* cp_arbiter;
    CollisionPhase phase;
    Node* space;

    Arbiter(CollisionPhase phase, SpaceNode* space_phys,
            cpArbiter* cp_arbiter);
};


uint8_t operator|(CollisionPhase phase, uint8_t other);
uint8_t operator|(CollisionPhase phase, CollisionPhase other);
uint8_t operator&(CollisionPhase phase, uint8_t other);
uint8_t operator&(CollisionPhase phase, CollisionPhase other);


struct CollisionPair {
    Node* body_node;
    Node* hitbox_node;

    CollisionPair(BodyNode* body, HitboxNode* hitbox);
};


typedef std::function<uint8_t(const Arbiter,
                              const CollisionPair, const CollisionPair)> \
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

    void set_gravity(const glm::dvec2 gravity);
    glm::dvec2 get_gravity() const;

    void set_damping(const double damping);
    double get_damping() const;

    void set_sleeping_threshold(const double threshold);
    double get_sleeping_threshold() const;

    bool is_locked() const;
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

    void override_simulation_position();
    void sync_simulation_position() const;

    void override_simulation_rotation();
    void sync_simulation_rotation() const;

    void set_velocity(const glm::dvec2 velocity);
    glm::dvec2 get_velocity() const;

    void set_force(const glm::dvec2 force);
    glm::dvec2 get_force() const;

    void set_torque(const double torque);
    double get_torque() const;

    void set_angular_velocity(const double angular_velocity);
    double get_angular_velocity() const;

    bool is_sleeping() const;
    void sleep();
    void activate();
};


struct HitboxNode {
    cpShape* cp_shape = nullptr;

    void initialize();
    void destroy();
    void update_physics_shape();
    void attach_to_simulation();

    void set_trigger_id(const CollisionTriggerId trigger_id);
    CollisionTriggerId get_trigger_id() const;

    void set_group(const CollisionGroup group);
    CollisionGroup get_group() const;

    void set_mask(const CollisionBitmask mask);
    CollisionBitmask get_mask() const;

    void set_collision_mask(const CollisionBitmask mask);
    CollisionBitmask get_collision_mask() const;
};

} // namespace kaacore
