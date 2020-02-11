#pragma once

#include <functional>
#include <vector>

#include <chipmunk/chipmunk.h>
#include <glm/glm.hpp>

#include "kaacore/node_ptr.h"

namespace kaacore {

typedef size_t CollisionTriggerId;
typedef size_t CollisionGroup;
typedef cpBitmask CollisionBitmask;

constexpr uint32_t default_simulation_step_size = 10;

class Node;
class SpaceNode;
class BodyNode;
class HitboxNode;

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
    NodePtr space;

    Arbiter(CollisionPhase phase, SpaceNode* space_phys, cpArbiter* cp_arbiter);
};

uint8_t
operator|(CollisionPhase phase, uint8_t other);
uint8_t
operator|(CollisionPhase phase, CollisionPhase other);
uint8_t operator&(CollisionPhase phase, uint8_t other);
uint8_t operator&(CollisionPhase phase, CollisionPhase other);

struct CollisionPair {
    NodePtr body_node;
    NodePtr hitbox_node;

    CollisionPair(BodyNode* body, HitboxNode* hitbox);
};

typedef std::function<uint8_t(const Arbiter, CollisionPair, CollisionPair)>
    CollisionHandlerFunc;

typedef std::function<void(const SpaceNode*)> SpacePostStepFunc;

void
cp_call_post_step_callbacks(
    cpSpace* cp_space, void* space_node_phys_ptr, void* data);

class SpaceNode {
    friend class Node;
    friend class BodyNode;
    friend class HitboxNode;
    friend class Scene;
    friend void cp_call_post_step_callbacks(cpSpace*, void*, void*);

    cpSpace* _cp_space = nullptr;
    uint32_t _time_acc = 0;
    std::vector<SpacePostStepFunc> _post_step_callbacks;

    SpaceNode();
    ~SpaceNode();

    void simulate(uint32_t dt);

  public:
    void add_post_step_callback(const SpacePostStepFunc& func);

    void set_collision_handler(
        CollisionTriggerId trigger_a, CollisionTriggerId trigger_b,
        CollisionHandlerFunc handler,
        uint8_t phases_mask = uint8_t(CollisionPhase::any_phase),
        bool only_non_deleted_nodes = true);

    void gravity(const glm::dvec2& gravity);
    glm::dvec2 gravity();

    void damping(const double& damping);
    double damping();

    void sleeping_threshold(const double& threshold);
    double sleeping_threshold();

    bool locked() const;
};

enum struct BodyNodeType {
    dynamic = cpBodyType::CP_BODY_TYPE_DYNAMIC,
    kinematic = cpBodyType::CP_BODY_TYPE_KINEMATIC,
    static_ = cpBodyType::CP_BODY_TYPE_STATIC
};

class BodyNode {
    friend class Node;
    friend class HitboxNode;
    friend class Scene;

    cpBody* _cp_body = nullptr;

    BodyNode();
    ~BodyNode();

    void attach_to_simulation();

    void override_simulation_position();
    void sync_simulation_position() const;

    void override_simulation_rotation();
    void sync_simulation_rotation() const;

  public:
    SpaceNode* space() const;

    void body_type(const BodyNodeType& type);
    BodyNodeType body_type();

    void mass(const double& m);
    double mass();

    void moment(const double& i);
    double moment();

    void velocity(const glm::dvec2& velocity);
    glm::dvec2 velocity();

    void force(const glm::dvec2& force);
    glm::dvec2 force();

    void torque(const double& torque);
    double torque();

    void angular_velocity(const double& angular_velocity);
    double angular_velocity();

    bool sleeping();
    void sleeping(const bool& sleeping);
};

class HitboxNode {
    friend class Node;

    cpShape* _cp_shape = nullptr;

    HitboxNode();
    ~HitboxNode();

    void update_physics_shape();
    void attach_to_simulation();

  public:
    SpaceNode* space() const;

    void trigger_id(const CollisionTriggerId& trigger_id);
    CollisionTriggerId trigger_id();

    void group(const CollisionGroup& group);
    CollisionGroup group();

    void mask(const CollisionBitmask& mask);
    CollisionBitmask mask();

    void collision_mask(const CollisionBitmask& mask);
    CollisionBitmask collision_mask();
};

} // namespace kaacore
