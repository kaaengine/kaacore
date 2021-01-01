#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <vector>

#include <chipmunk/chipmunk.h>
#include <glm/glm.hpp>

#include "kaacore/clock.h"
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

constexpr HighPrecisionDuration default_simulation_step_size = 10000us; // 0.01s

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

struct SpatialQueryResultBase {
    NodePtr body_node;
    NodePtr hitbox_node;

    SpatialQueryResultBase() = default;
    SpatialQueryResultBase(const cpShape* cp_shape);
};

struct CollisionContactPoint {
    glm::dvec2 point_a;
    glm::dvec2 point_b;
    double distance;
};

struct ShapeQueryResult : SpatialQueryResultBase {
    std::vector<CollisionContactPoint> contact_points;

    ShapeQueryResult() = default;
    ShapeQueryResult(const cpShape* cp_shape, const cpContactPointSet* points);
};

struct RayQueryResult : SpatialQueryResultBase {
    glm::dvec2 point;
    glm::dvec2 normal;
    double alpha;

    RayQueryResult() = default;
    RayQueryResult(
        const cpShape* cp_shape, const cpVect point, const cpVect normal,
        const double alpha);
};

struct PointQueryResult : SpatialQueryResultBase {
    glm::dvec2 point;
    double distance;

    PointQueryResult() = default;
    PointQueryResult(
        const cpShape* cp_shape, const cpVect point, const double distance);
};

class SpaceNode {
  public:
    void add_post_step_callback(const SpacePostStepFunc& func);

    void set_collision_handler(
        CollisionTriggerId trigger_a, CollisionTriggerId trigger_b,
        CollisionHandlerFunc handler,
        uint8_t phases_mask = uint8_t(CollisionPhase::any_phase),
        bool only_non_deleted_nodes = true);

    const std::vector<ShapeQueryResult> query_shape_overlaps(
        const Shape& shape, const CollisionBitmask mask = collision_bitmask_all,
        const CollisionBitmask collision_mask = collision_bitmask_all,
        const CollisionGroup group = collision_group_none);

    const std::vector<RayQueryResult> query_ray(
        const glm::dvec2 ray_start, const glm::dvec2 ray_end,
        const double radius = 0.,
        const CollisionBitmask mask = collision_bitmask_all,
        const CollisionBitmask collision_mask = collision_bitmask_all,
        const CollisionGroup group = collision_group_none);

    const std::vector<PointQueryResult> query_point_neighbors(
        const glm::dvec2 point, const double max_distance,
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

  private:
    SpaceNode();
    ~SpaceNode();

    void simulate(const HighPrecisionDuration dt);

    cpSpace* _cp_space = nullptr;
    HighPrecisionDuration _time_acc = 0us;
    std::vector<SpacePostStepFunc> _post_step_callbacks;

    friend class Node;
    friend class BodyNode;
    friend class HitboxNode;
    friend class Scene;
    friend void cp_call_post_step_callbacks(cpSpace*, void*, void*);
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

  private:
    BodyNode();
    ~BodyNode();

    void attach_to_simulation();
    void detach_from_simulation();

    void override_simulation_position();
    void sync_simulation_position() const;

    void override_simulation_rotation();
    void sync_simulation_rotation() const;

    cpBody* _cp_body = nullptr;

    std::optional<double> _damping = std::nullopt;
    std::optional<cpVect> _gravity = std::nullopt;

    VelocityUpdateCallback _velocity_update_callback = nullptr;
    PositionUpdateCallback _position_update_callback = nullptr;

    friend class Node;
    friend class HitboxNode;
    friend class Scene;

    friend void _velocity_update_wrapper(cpBody*, cpVect, cpFloat, cpFloat);
    friend void _position_update_wrapper(cpBody*, cpFloat);
};

CpShapeUniquePtr
prepare_hitbox_shape(const Shape& shape, const Transformation& transformtion);

class HitboxNode {
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

    void sensor(const bool sensor);
    bool sensor();

    void elasticity(const double elasticity);
    double elasticity();

    void friction(const double friction);
    double friction();

    void surface_velocity(const glm::dvec2 surface_velocity);
    glm::dvec2 surface_velocity();

  private:
    HitboxNode();
    ~HitboxNode();

    void update_physics_shape();
    void attach_to_simulation();
    void detach_from_simulation();

    cpShape* _cp_shape = nullptr;

    friend class Node;
};

} // namespace kaacore
