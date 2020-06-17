#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <chipmunk/chipmunk.h>
#include <glm/glm.hpp>

#include "kaacore/geometry.h"
#include "kaacore/node_ptr.h"
#include "kaacore/shapes.h"

namespace kaacore {

typedef size_t CollisionTriggerId;
typedef size_t CollisionGroup;
typedef cpBitmask CollisionBitmask;

constexpr CollisionGroup collision_group_none = CP_NO_GROUP;
constexpr CollisionBitmask collision_bitmask_all = CP_ALL_CATEGORIES;
constexpr CollisionBitmask collision_bitmask_none = ~CP_ALL_CATEGORIES;

typedef std::unique_ptr<cpShape, void (*)(cpShape*)> CpShapeUniquePtr;

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

struct CollisionContactPoint {
    glm::dvec2 point_a;
    glm::dvec2 point_b;
    double distance;
};

struct ShapeQueryResult {
    NodePtr body_node;
    NodePtr hitbox_node;
    std::vector<CollisionContactPoint> contact_points;
};

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

    const std::vector<ShapeQueryResult> query_shape_overlaps(
        const Shape& shape, const glm::dvec2& position = {0., 0.},
        const CollisionBitmask mask = collision_bitmask_all,
        const CollisionBitmask collision_mask = collision_bitmask_all,
        const CollisionGroup group = collision_group_none);

    void gravity(const glm::dvec2& gravity);
    glm::dvec2 gravity();

    void damping(const double damping);
    double damping();

    void sleeping_threshold(const double threshold);
    double sleeping_threshold();

    bool locked() const;
};

enum struct BodyNodeType {
    dynamic = cpBodyType::CP_BODY_TYPE_DYNAMIC,
    kinematic = cpBodyType::CP_BODY_TYPE_KINEMATIC,
    static_ = cpBodyType::CP_BODY_TYPE_STATIC
};

typedef std::function<void(Node*, glm::dvec2, double, double)>
    VelocityUpdateCallback;
static void
_velocity_update_wrapper(
    cpBody* cp_body, cpVect gravity, cpFloat damping, cpFloat dt);

typedef std::function<void(Node*, double)> PositionUpdateCallback;
static void
_position_update_wrapper(cpBody* cp_body, cpFloat dt);

class BodyNode {
    friend class Node;
    friend class HitboxNode;
    friend class Scene;
    friend void _velocity_update_wrapper(cpBody*, cpVect, cpFloat, cpFloat);
    friend void _position_update_wrapper(cpBody*, cpFloat);

    cpBody* _cp_body = nullptr;

    std::optional<double> _damping = std::nullopt;
    std::optional<cpVect> _gravity = std::nullopt;

    VelocityUpdateCallback _velocity_update_callback = nullptr;
    PositionUpdateCallback _position_update_callback = nullptr;

    BodyNode();
    ~BodyNode();

    void attach_to_simulation();
    void detach_from_simulation();

    void override_simulation_position();
    void sync_simulation_position() const;

    void override_simulation_rotation();
    void sync_simulation_rotation() const;

  public:
    SpaceNode* space() const;

    void body_type(const BodyNodeType& type);
    BodyNodeType body_type();

    void mass(const double m);
    double mass();
    double mass_inverse();

    void moment(const double i);
    double moment();
    double moment_inverse();

    void center_of_gravity(const glm::dvec2& cog);
    glm::dvec2 center_of_gravity();

    void velocity(const glm::dvec2& velocity);
    glm::dvec2 velocity();

    void local_force(const glm::dvec2& force);
    glm::dvec2 local_force();
    void force(const glm::dvec2& force);
    glm::dvec2 force();
    void apply_force_at_local(
        const glm::dvec2& force, const glm::dvec2& at) const;
    void apply_impulse_at_local(
        const glm::dvec2& force, const glm::dvec2& at) const;
    void apply_force_at(const glm::dvec2& force, const glm::dvec2& at) const;
    void apply_impulse_at(const glm::dvec2& force, const glm::dvec2& at) const;

    void torque(const double torque);
    double torque();

    void angular_velocity(const double& angular_velocity);
    double angular_velocity();

    void damping(const std::optional<double>& damping);
    std::optional<double> damping();

    void gravity(const std::optional<glm::dvec2>& gravity);
    std::optional<glm::dvec2> gravity();

    bool sleeping();
    void sleeping(const bool sleeping);

    void _velocity_bias(const glm::dvec2& velocity);
    glm::dvec2 _velocity_bias();

    void _angular_velocity_bias(const double torque);
    double _angular_velocity_bias();

    void set_velocity_update_callback(VelocityUpdateCallback callback);
    void set_position_update_callback(PositionUpdateCallback callback);
};

CpShapeUniquePtr
prepare_hitbox_shape(const Shape& shape, const Transformation& transformtion);

class HitboxNode {
    friend class Node;

    cpShape* _cp_shape = nullptr;

    HitboxNode();
    ~HitboxNode();

    void update_physics_shape();
    void attach_to_simulation();
    void detach_from_simulation();

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
