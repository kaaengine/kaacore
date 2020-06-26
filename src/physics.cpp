#include <type_traits>

#include <chipmunk/chipmunk.h>

#ifndef _MSC_VER
extern "C"
{
#endif

// this header does not have 'extern "C"' on it's own
// but on Visual Studio it's built as C++
#include <chipmunk/chipmunk_private.h>

#ifndef _MSC_VER
}
#endif

#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/exceptions.h"
#include "kaacore/geometry.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/utils.h"

#include "kaacore/physics.h"

namespace kaacore {

// assertion helpers

#define ASSERT_VALID_SPACE_NODE(space_node)                                    \
    KAACORE_ASSERT(                                                            \
        container_node(space_node)->_type == NodeType::space,                  \
        "Invalid type - space type expected.");                                \
    KAACORE_ASSERT(                                                            \
        space_node->_cp_space != nullptr,                                      \
        "Space node has invalid internal state.");

#define ASSERT_VALID_BODY_NODE(body_node)                                      \
    KAACORE_ASSERT(                                                            \
        container_node(body_node)->_type == NodeType::body,                    \
        "Invalid type - body type expected.");                                 \
    KAACORE_ASSERT(                                                            \
        body_node->_cp_body != nullptr,                                        \
        "Body node has invalid internal state.");

#define ASSERT_DYNAMIC_BODY_NODE(body_node)                                    \
    ASSERT_VALID_BODY_NODE(body_node);                                         \
    KAACORE_ASSERT(                                                            \
        body_node->body_type() == BodyNodeType::dynamic,                       \
        "Invalid body type - dynamic body type expected.");

#define ASSERT_VALID_HITBOX_NODE(hitbox_node)                                  \
    KAACORE_ASSERT(                                                            \
        container_node(hitbox_node)->_type == NodeType::hitbox,                \
        "Invalid type = hitbox type expected.");                               \
    KAACORE_ASSERT(                                                            \
        hitbox_node->_cp_shape != nullptr,                                     \
        "Hitbox node has invalid internal state.");

// conversions

cpVect
convert_vector(const glm::dvec2 vec)
{
    return {vec.x, vec.y};
}

glm::dvec2
convert_vector(const cpVect vec)
{
    return {vec.x, vec.y};
}

std::vector<CollisionContactPoint>
convert_contact_points(const cpContactPointSet* const cp_contact_points)
{
    std::vector<CollisionContactPoint> contact_points{};
    contact_points.reserve(cp_contact_points->count);
    for (int i = 0; i < cp_contact_points->count; i++) {
        contact_points.push_back(CollisionContactPoint{
            convert_vector(cp_contact_points->points[i].pointA),
            convert_vector(cp_contact_points->points[i].pointB),
            cp_contact_points->points[i].distance});
    }

    return contact_points;
}

inline constexpr Node*
container_node(const SpaceNode* space)
{
    return container_of(space, &Node::space);
}

inline constexpr Node*
container_node(const BodyNode* body)
{
    return container_of(body, &Node::body);
}

inline constexpr Node*
container_node(const HitboxNode* hitbox)
{
    return container_of(hitbox, &Node::hitbox);
}

void
space_safe_call(SpaceNode* space_node_phys, const SpacePostStepFunc& func)
{
    if (space_node_phys and space_node_phys->locked()) {
        space_node_phys->add_post_step_callback(func);
    } else {
        func(space_node_phys);
    }
}

void
space_safe_call(Node* space_node, const SpacePostStepFunc& func)
{
    KAACORE_ASSERT(
        space_node->type() == NodeType::space,
        "Invalid type - space type expected.");
    space_safe_call(&space_node->space, func);
}

Arbiter::Arbiter(
    CollisionPhase phase, SpaceNode* space_phys, cpArbiter* cp_arbiter)
    : phase(phase), cp_arbiter(cp_arbiter), space(container_node(space_phys))
{}

CollisionPair::CollisionPair(BodyNode* body, HitboxNode* hitbox)
    : body_node(container_node(body)), hitbox_node(container_node(hitbox))
{}

uint8_t
operator|(CollisionPhase phase, uint8_t other)
{
    return uint8_t(phase) | other;
}

uint8_t
operator|(CollisionPhase phase, CollisionPhase other)
{
    return uint8_t(phase) | uint8_t(other);
}

uint8_t operator&(CollisionPhase phase, uint8_t other)
{
    return uint8_t(phase) & other;
}

uint8_t operator&(CollisionPhase phase, CollisionPhase other)
{
    return uint8_t(phase) & uint8_t(other);
}

SpaceNode::SpaceNode()
{
    this->_cp_space = cpSpaceNew();
    log<LogLevel::debug>(
        "Creating space node %p (cpSpace: %p)", container_node(this),
        this->_cp_space);
    cpSpaceSetUserData(this->_cp_space, this);
    this->_time_acc = 0;
}

void
_release_cp_collision_handler_callback(cpCollisionHandler* cp_collision_handler)
{
    KAACORE_ASSERT_TERMINATE(
        cp_collision_handler->userData != nullptr,
        "Invalid internal state of collision callback.");
    auto collision_handler_func =
        static_cast<CollisionHandlerFunc*>(cp_collision_handler->userData);
    delete collision_handler_func;
}

SpaceNode::~SpaceNode()
{
    log<LogLevel::debug>(
        "Destroying space node %p (cpSpace: %p)", container_node(this),
        this->_cp_space);

    KAACORE_ASSERT_TERMINATE(
        this->_cp_space != nullptr, "Invalid internal state of space node.");

    // deleting allocated collision handlers
    cpHashSetEach(
        this->_cp_space->collisionHandlers,
        [](void* elt, void* data) {
            auto cp_handler = static_cast<cpCollisionHandler*>(elt);
            // we assume that every collision handler is created by kaacore
            // since we blindly cast void* to CollisionHandlerFunc*
            _release_cp_collision_handler_callback(cp_handler);
        },
        nullptr);

    cpSpaceFree(this->_cp_space);
    this->_cp_space = nullptr;
}

void
cp_call_post_step_callbacks(
    cpSpace* cp_space, void* space_node_phys_ptr, void* data)
{
    SpaceNode* space_node_phys = static_cast<SpaceNode*>(space_node_phys_ptr);
    for (const auto& func : space_node_phys->_post_step_callbacks) {
        func(space_node_phys);
    }
    space_node_phys->_post_step_callbacks.clear();
}

void
SpaceNode::add_post_step_callback(const SpacePostStepFunc& func)
{
    if (this->_post_step_callbacks.empty()) {
        cpSpaceAddPostStepCallback(
            this->_cp_space, cp_call_post_step_callbacks, this, nullptr);
    }
    this->_post_step_callbacks.push_back(func);
}

void
SpaceNode::simulate(const uint32_t dt)
{
    ASSERT_VALID_SPACE_NODE(this);
    log<LogLevel::debug, LogCategory::physics>(
        "Simulating SpaceNode(%p) physics, dt = %lu", this, dt);
    uint32_t time_left = dt + this->_time_acc;
    while (time_left > default_simulation_step_size) {
        cpSpaceStep(this->_cp_space, 0.001 * default_simulation_step_size);
        time_left -= default_simulation_step_size;
    }
    this->_time_acc = time_left;
}

template<typename R_type, CollisionPhase phase, bool non_null_nodes>
R_type
_chipmunk_collision_handler(
    cpArbiter* cp_arbiter, cpSpace* cp_space, cpDataPointer data)
{
    auto func = static_cast<CollisionHandlerFunc*>(data);

    cpBody* cp_body_a = nullptr;
    cpBody* cp_body_b = nullptr;
    cpShape* cp_shape_a = nullptr;
    cpShape* cp_shape_b = nullptr;
    cpArbiterGetBodies(cp_arbiter, &cp_body_a, &cp_body_b);
    cpArbiterGetShapes(cp_arbiter, &cp_shape_a, &cp_shape_b);

    KAACORE_ASSERT(cp_body_a != nullptr, "Invalid state of body A.");
    KAACORE_ASSERT(cp_body_b != nullptr, "Invalid state of body B.");
    KAACORE_ASSERT(cp_shape_a != nullptr, "Invalid state of shape A.");
    KAACORE_ASSERT(cp_shape_b != nullptr, "Invalid state of shape B.");

    auto body_a = static_cast<BodyNode*>(cpBodyGetUserData(cp_body_a));
    auto body_b = static_cast<BodyNode*>(cpBodyGetUserData(cp_body_b));
    auto hitbox_a = static_cast<HitboxNode*>(cpShapeGetUserData(cp_shape_a));
    auto hitbox_b = static_cast<HitboxNode*>(cpShapeGetUserData(cp_shape_b));

    auto space_phys = static_cast<SpaceNode*>(cpSpaceGetUserData(cp_space));

    if (non_null_nodes and (body_a == nullptr or body_b == nullptr or
                            hitbox_a == nullptr or hitbox_b == nullptr)) {
        return R_type(0);
    }

    return (R_type)(*func)(
        Arbiter(phase, space_phys, cp_arbiter), CollisionPair(body_a, hitbox_a),
        CollisionPair(body_b, hitbox_b));
}

template<typename R_type>
R_type
_chipmunk_collision_noop(
    cpArbiter* cp_arbiter, cpSpace* cp_space, cpDataPointer data)
{
    return R_type(1);
}

void
SpaceNode::set_collision_handler(
    CollisionTriggerId trigger_a, CollisionTriggerId trigger_b,
    CollisionHandlerFunc handler, uint8_t phases_mask,
    bool only_non_deleted_nodes)
{
    cpCollisionHandler* cp_handler = cpSpaceAddCollisionHandler(
        this->_cp_space, static_cast<cpCollisionType>(trigger_a),
        static_cast<cpCollisionType>(trigger_b));

    if (cp_handler->userData != nullptr) {
        _release_cp_collision_handler_callback(cp_handler);
    }
    CollisionHandlerFunc* func = new CollisionHandlerFunc();
    *func = handler;
    cp_handler->userData = func;

    cp_handler->beginFunc = _chipmunk_collision_noop<uint8_t>;
    cp_handler->preSolveFunc = _chipmunk_collision_noop<uint8_t>;
    cp_handler->postSolveFunc = _chipmunk_collision_noop<void>;
    cp_handler->separateFunc = _chipmunk_collision_noop<void>;

    if (only_non_deleted_nodes) {
        if (CollisionPhase::begin & phases_mask) {
            cp_handler->beginFunc = &_chipmunk_collision_handler<
                uint8_t, CollisionPhase::begin, true>;
        }
        if (CollisionPhase::pre_solve & phases_mask) {
            cp_handler->preSolveFunc = &_chipmunk_collision_handler<
                uint8_t, CollisionPhase::pre_solve, true>;
        }
        if (CollisionPhase::post_solve & phases_mask) {
            cp_handler->postSolveFunc = &_chipmunk_collision_handler<
                void, CollisionPhase::post_solve, true>;
        }
        if (CollisionPhase::separate & phases_mask) {
            cp_handler->separateFunc = &_chipmunk_collision_handler<
                void, CollisionPhase::separate, true>;
        }
    } else {
        if (CollisionPhase::begin & phases_mask) {
            cp_handler->beginFunc = &_chipmunk_collision_handler<
                uint8_t, CollisionPhase::begin, false>;
        }
        if (CollisionPhase::pre_solve & phases_mask) {
            cp_handler->preSolveFunc = &_chipmunk_collision_handler<
                uint8_t, CollisionPhase::pre_solve, false>;
        }
        if (CollisionPhase::post_solve & phases_mask) {
            cp_handler->postSolveFunc = &_chipmunk_collision_handler<
                void, CollisionPhase::post_solve, false>;
        }
        if (CollisionPhase::separate & phases_mask) {
            cp_handler->separateFunc = &_chipmunk_collision_handler<
                void, CollisionPhase::separate, false>;
        }
    }
}

void
_cp_space_query_shape_callback(
    cpShape* cp_shape, cpContactPointSet* points, void* data)
{
    auto results = reinterpret_cast<std::vector<ShapeQueryResult>*>(data);
    cpBody* cp_body = cpShapeGetBody(cp_shape);

    Node* body_node;
    Node* hitbox_node;

    if (cp_body) {
        KAACORE_ASSERT(
            cpBodyGetUserData(cp_body) != nullptr,
            "Invalid internal state of body node.");
        body_node =
            container_node(static_cast<BodyNode*>(cpBodyGetUserData(cp_body)));
    }

    KAACORE_ASSERT(
        cpShapeGetUserData(cp_shape) != nullptr,
        "Invalid internal state of shape.");
    hitbox_node =
        container_node(static_cast<HitboxNode*>(cpShapeGetUserData(cp_shape)));

    results->push_back(
        {body_node, hitbox_node, convert_contact_points(points)});
}

const std::vector<ShapeQueryResult>
SpaceNode::query_shape_overlaps(
    const Shape& shape, const glm::dvec2& position, const CollisionBitmask mask,
    const CollisionBitmask collision_mask, const CollisionGroup group)
{
    std::vector<ShapeQueryResult> results;
    auto shape_uptr =
        prepare_hitbox_shape(shape, Transformation::translate(position));
    auto filter = cpShapeGetFilter(shape_uptr.get());
    filter.categories = mask;
    filter.mask = collision_mask;
    filter.group = group;
    cpShapeSetFilter(shape_uptr.get(), filter);

    // shapes are created without BB set up, cpShapeUpdate refreshes it
    cpShapeUpdate(shape_uptr.get(), cpTransformIdentity);

    cpSpaceShapeQuery(
        this->_cp_space, shape_uptr.get(), _cp_space_query_shape_callback,
        &results);

    return results;
}

glm::dvec2
SpaceNode::gravity()
{
    ASSERT_VALID_SPACE_NODE(this);
    return convert_vector(cpSpaceGetGravity(this->_cp_space));
}

void
SpaceNode::gravity(const glm::dvec2& gravity)
{
    ASSERT_VALID_SPACE_NODE(this);
    cpSpaceSetGravity(this->_cp_space, convert_vector(gravity));
}

double
SpaceNode::damping()
{
    ASSERT_VALID_SPACE_NODE(this);
    return cpSpaceGetDamping(this->_cp_space);
}

void
SpaceNode::damping(const double damping)
{
    ASSERT_VALID_SPACE_NODE(this);
    cpSpaceSetDamping(this->_cp_space, damping);
}

double
SpaceNode::sleeping_threshold()
{
    ASSERT_VALID_SPACE_NODE(this);
    return cpSpaceGetSleepTimeThreshold(this->_cp_space);
}

void
SpaceNode::sleeping_threshold(const double threshold)
{
    ASSERT_VALID_SPACE_NODE(this);
    cpSpaceSetSleepTimeThreshold(this->_cp_space, threshold);
}

bool
SpaceNode::locked() const
{
    ASSERT_VALID_SPACE_NODE(this);
    return cpSpaceIsLocked(this->_cp_space);
}

BodyNode::BodyNode()
{
    this->_cp_body = cpBodyNewKinematic();
    log<LogLevel::debug>(
        "Creating body node %p (cpBody: %p)", container_node(this),
        this->_cp_body);
    cpBodySetUserData(this->_cp_body, this);
    this->body_type(BodyNodeType::dynamic);
}

BodyNode::~BodyNode()
{
    this->detach_from_simulation();
}

void
BodyNode::attach_to_simulation()
{
    ASSERT_VALID_BODY_NODE(this);
    if (cpBodyGetSpace(this->_cp_body) == nullptr) {
        Node* node = container_node(this);
        log<LogLevel::debug>(
            "Attaching body node %p to simulation (space) (cpBody: %p)", node,
            this->_cp_body);
        KAACORE_ASSERT(
            node->_parent != nullptr,
            "Node must have a parent in order to attach it to the simulation");
        ASSERT_VALID_SPACE_NODE(static_cast<SpaceNode*>(&node->_parent->space));
        space_safe_call(node->_parent, [&](const SpaceNode* space_node_phys) {
            log<LogLevel::debug>(
                "Simulation callback: attaching cpBody %p", this->_cp_body);
            cpSpaceAddBody(space_node_phys->_cp_space, this->_cp_body);
        });
    }
}

void
BodyNode::detach_from_simulation()
{
    if (this->_cp_body != nullptr) {
        log<LogLevel::debug>(
            "Destroying body node %p (cpBody: %p)", container_node(this),
            this->_cp_body);
        cpBodySetUserData(this->_cp_body, nullptr);
        space_safe_call(
            this->space(),
            [body_ptr = this->_cp_body](const SpaceNode* space_node_phys) {
                log<LogLevel::debug>(
                    "Simulation callback: destroying cpBody %p", body_ptr);
                if (space_node_phys) {
                    cpSpaceRemoveBody(space_node_phys->_cp_space, body_ptr);
                }
                cpBodyFree(body_ptr);
            });
        this->_cp_body = nullptr;
    }
}

void
BodyNode::override_simulation_position()
{
    ASSERT_VALID_BODY_NODE(this);
    cpBodySetPosition(
        this->_cp_body, convert_vector(container_node(this)->_position));
}

void
BodyNode::sync_simulation_position() const
{
    ASSERT_VALID_BODY_NODE(this);
    container_node(this)->_set_position(
        convert_vector(cpBodyGetPosition(this->_cp_body)));
}

void
BodyNode::override_simulation_rotation()
{
    ASSERT_VALID_BODY_NODE(this);
    cpBodySetAngle(this->_cp_body, container_node(this)->_rotation);
}

void
BodyNode::sync_simulation_rotation() const
{
    ASSERT_VALID_BODY_NODE(this);
    container_node(this)->_set_rotation(cpBodyGetAngle(this->_cp_body));
}

SpaceNode*
BodyNode::space() const
{
    if (this->_cp_body) {
        cpSpace* cp_space = cpBodyGetSpace(this->_cp_body);
        if (cp_space) {
            return static_cast<SpaceNode*>(cpSpaceGetUserData(cp_space));
        }
    }

    return nullptr;
}

BodyNodeType
BodyNode::body_type()
{
    ASSERT_VALID_BODY_NODE(this);
    return static_cast<BodyNodeType>(cpBodyGetType(this->_cp_body));
}

void
BodyNode::body_type(const BodyNodeType& type)
{
    ASSERT_VALID_BODY_NODE(this);
    cpBodySetType(this->_cp_body, static_cast<cpBodyType>(type));

    if (type == BodyNodeType::dynamic) {
        if (this->mass() == 0.) {
            this->mass(20.);
        }
        if (this->moment() == 0.) {
            this->moment(10000.);
        }
    }
}

double
BodyNode::mass()
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    return cpBodyGetMass(this->_cp_body);
}

double
BodyNode::mass_inverse()
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    return this->_cp_body->m_inv;
}

void
BodyNode::mass(const double m)
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    cpBodySetMass(this->_cp_body, m);
}

double
BodyNode::moment()
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    return cpBodyGetMoment(this->_cp_body);
}

double
BodyNode::moment_inverse()
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    return this->_cp_body->i_inv;
}

void
BodyNode::moment(const double i)
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    cpBodySetMoment(this->_cp_body, i);
}

void
BodyNode::center_of_gravity(const glm::dvec2& cog)
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    cpBodySetCenterOfGravity(this->_cp_body, convert_vector(cog));
}

glm::dvec2
BodyNode::center_of_gravity()
{
    ASSERT_DYNAMIC_BODY_NODE(this);
    return convert_vector(cpBodyGetCenterOfGravity(this->_cp_body));
}

glm::dvec2
BodyNode::velocity()
{
    ASSERT_VALID_BODY_NODE(this);
    return convert_vector(cpBodyGetVelocity(this->_cp_body));
}

void
BodyNode::velocity(const glm::dvec2& velocity)
{
    ASSERT_VALID_BODY_NODE(this);
    cpBodySetVelocity(this->_cp_body, convert_vector(velocity));
}

glm::dvec2
BodyNode::local_force()
{
    ASSERT_VALID_BODY_NODE(this);
    auto cp_vector = cpBodyGetForce(this->_cp_body);
    auto transformed_cp_vector = cpTransformVect(
        cpTransformInverse(this->_cp_body->transform), cp_vector);
    return convert_vector(transformed_cp_vector);
}

void
BodyNode::local_force(const glm::dvec2& force)
{
    auto transformed_force =
        cpTransformVect(this->_cp_body->transform, convert_vector(force));
    cpBodySetForce(this->_cp_body, transformed_force);
}

glm::dvec2
BodyNode::force()
{
    ASSERT_VALID_BODY_NODE(this);
    return convert_vector(cpBodyGetForce(this->_cp_body));
}

void
BodyNode::force(const glm::dvec2& force)
{
    cpBodySetForce(this->_cp_body, convert_vector(force));
}

void
BodyNode::apply_force_at_local(
    const glm::dvec2& force, const glm::dvec2& at) const
{
    cpBodyApplyForceAtLocalPoint(
        this->_cp_body, convert_vector(force), convert_vector(at));
}

void
BodyNode::apply_impulse_at_local(
    const glm::dvec2& force, const glm::dvec2& at) const
{
    cpBodyApplyImpulseAtLocalPoint(
        this->_cp_body, convert_vector(force), convert_vector(at));
}

void
BodyNode::apply_force_at(const glm::dvec2& force, const glm::dvec2& at) const
{
    cpBodyApplyForceAtWorldPoint(
        this->_cp_body, convert_vector(force), convert_vector(at));
}

void
BodyNode::apply_impulse_at(const glm::dvec2& force, const glm::dvec2& at) const
{
    cpBodyApplyImpulseAtWorldPoint(
        this->_cp_body, convert_vector(force), convert_vector(at));
}

double
BodyNode::torque()
{
    ASSERT_VALID_BODY_NODE(this);
    return cpBodyGetTorque(this->_cp_body);
}

void
BodyNode::torque(const double torque)
{
    ASSERT_VALID_BODY_NODE(this);
    cpBodySetTorque(this->_cp_body, torque);
}

double
BodyNode::angular_velocity()
{
    ASSERT_VALID_BODY_NODE(this);
    return cpBodyGetAngularVelocity(this->_cp_body);
}

void
BodyNode::angular_velocity(const double& angular_velocity)
{
    ASSERT_VALID_BODY_NODE(this);
    cpBodySetAngularVelocity(this->_cp_body, angular_velocity);
}

void
BodyNode::damping(const std::optional<double>& damping)
{
    this->_damping = damping;
    if (this->_damping) {
        cpBodySetVelocityUpdateFunc(this->_cp_body, _velocity_update_wrapper);
    } else if (
        this->_gravity == std::nullopt and
        this->_velocity_update_callback == nullptr) {
        // if custom velocity wrapper is not in use anymore, switch back to
        // default chipmunk function
        cpBodySetVelocityUpdateFunc(this->_cp_body, cpBodyUpdateVelocity);
    }
}

std::optional<double>
BodyNode::damping()
{
    return this->_damping;
}

void
BodyNode::gravity(const std::optional<glm::dvec2>& gravity)
{
    if (gravity) {
        this->_gravity = convert_vector(gravity.value());
        cpBodySetVelocityUpdateFunc(this->_cp_body, _velocity_update_wrapper);
    } else {
        this->_gravity = std::nullopt;

        if (this->_damping == std::nullopt and
            this->_velocity_update_callback == nullptr) {
            // if custom velocity wrapper is not in use anymore, switch back to
            // default chipmunk function
            cpBodySetVelocityUpdateFunc(this->_cp_body, cpBodyUpdateVelocity);
        }
    }
}

std::optional<glm::dvec2>
BodyNode::gravity()
{
    if (this->_gravity) {
        return convert_vector(this->_gravity.value());
    }
    return std::nullopt;
}

bool
BodyNode::sleeping()
{
    ASSERT_VALID_BODY_NODE(this);
    return cpBodyIsSleeping(this->_cp_body);
}

void
BodyNode::sleeping(const bool sleeping)
{
    ASSERT_VALID_BODY_NODE(this);
    if (sleeping) {
        cpBodySleep(this->_cp_body);
    } else {
        cpBodyActivate(this->_cp_body);
    }
}

glm::dvec2
BodyNode::_velocity_bias()
{
    return convert_vector(this->_cp_body->v_bias);
}

void
BodyNode::_velocity_bias(const glm::dvec2& velocity)
{
    this->_cp_body->v_bias = convert_vector(velocity);
}

double
BodyNode::_angular_velocity_bias()
{
    return this->_cp_body->w_bias;
}

void
BodyNode::_angular_velocity_bias(const double torque)
{
    this->_cp_body->w_bias = torque;
}

void
_velocity_update_wrapper(
    cpBody* cp_body, cpVect gravity, cpFloat damping, cpFloat dt)
{
    auto* body = static_cast<BodyNode*>(cpBodyGetUserData(cp_body));

    if (body->_gravity) {
        gravity = body->_gravity.value();
    }

    if (body->_damping) {
        damping = body->_damping.value();
    }

    if (body->_velocity_update_callback == nullptr) {
        return cpBodyUpdateVelocity(cp_body, gravity, damping, dt);
    }

    Node* node = container_node(body);
    body->_velocity_update_callback(node, {gravity.x, gravity.y}, damping, dt);
    // TODO: cpAssertSaneBody
}

void
_position_update_wrapper(cpBody* cp_body, cpFloat dt)
{
    auto* body = static_cast<BodyNode*>(cpBodyGetUserData(cp_body));
    Node* node = container_node(body);
    body->_position_update_callback(node, dt);
    // TODO: cpAssertSaneBody
}

void
BodyNode::set_velocity_update_callback(VelocityUpdateCallback callback)
{
    this->_velocity_update_callback = std::move(callback);
    if (this->_velocity_update_callback == nullptr) {
        return cpBodySetVelocityUpdateFunc(
            this->_cp_body, cpBodyUpdateVelocity);
    }
    cpBodySetVelocityUpdateFunc(this->_cp_body, _velocity_update_wrapper);
}

void
BodyNode::set_position_update_callback(PositionUpdateCallback callback)
{
    this->_position_update_callback = std::move(callback);
    if (this->_position_update_callback == nullptr) {
        // if custom position wrapper is not in use anymore, switch back to
        // default chipmunk function
        return cpBodySetPositionUpdateFunc(
            this->_cp_body, cpBodyUpdatePosition);
    }

    cpBodySetPositionUpdateFunc(this->_cp_body, _position_update_wrapper);
}

CpShapeUniquePtr
prepare_hitbox_shape(const Shape& shape, const Transformation& transformation)
{
    KAACORE_ASSERT(shape.type != ShapeType::none, "Hitbox must have a shape.");
    KAACORE_ASSERT(
        shape.type != ShapeType::freeform,
        "Hitbox must not have a freeform shape.");

    auto transformed_shape = shape.transform(transformation);

    cpShape* shape_ptr = nullptr;
    const auto cp_points =
        reinterpret_cast<const cpVect*>(transformed_shape.points.data());

    if (shape.type == ShapeType::segment) {
        KAACORE_ASSERT(
            shape.points.size() == 2,
            "Invalid number of points for segment shape.");
        shape_ptr = cpSegmentShapeNew(
            nullptr, cp_points[0], cp_points[1], transformed_shape.radius);
    } else if (shape.type == ShapeType::circle) {
        KAACORE_ASSERT(
            shape.points.size() == 1,
            "Invalid number of points for circle shape.");
        shape_ptr =
            cpCircleShapeNew(nullptr, transformed_shape.radius, cp_points[0]);
    } else if (shape.type == ShapeType::polygon) {
        shape_ptr =
            cpPolyShapeNewRaw(nullptr, shape.points.size(), cp_points, 0.);
    }
    KAACORE_ASSERT(shape_ptr != nullptr, "Unsupported shape.");

    return CpShapeUniquePtr{shape_ptr, cpShapeFree};
}

HitboxNode::HitboxNode() {}

HitboxNode::~HitboxNode()
{
    this->detach_from_simulation();
}

SpaceNode*
HitboxNode::space() const
{
    if (this->_cp_shape) {
        cpSpace* cp_space = cpShapeGetSpace(this->_cp_shape);
        if (cp_space) {
            return static_cast<SpaceNode*>(cpSpaceGetUserData(cp_space));
        }
    }

    return nullptr;
}

void
HitboxNode::update_physics_shape()
{
    Node* node = container_node(this);
    cpShape* new_cp_shape;

    const auto transformation =
        Transformation() | Transformation::translate(node->_position) |
        Transformation::rotate(node->_rotation) |
        Transformation::scale(
            node->_scale *
            (node->_parent ? node->_parent->_scale : glm::dvec2(1.)));

    new_cp_shape = prepare_hitbox_shape(node->_shape, transformation).release();

    log<LogLevel::debug>(
        "Updating hitbox node %p shape (cpShape: %p)", node, new_cp_shape);

    cpShapeSetUserData(new_cp_shape, this);

    if (this->_cp_shape != nullptr) {
        cpShapeSetUserData(this->_cp_shape, nullptr);

        // copy over existing cpShape parameters
        cpShapeSetCollisionType(
            new_cp_shape, cpShapeGetCollisionType(this->_cp_shape));
        cpShapeSetFilter(new_cp_shape, cpShapeGetFilter(this->_cp_shape));
        cpShapeSetSensor(new_cp_shape, cpShapeGetSensor(this->_cp_shape));
        cpShapeSetElasticity(
            new_cp_shape, cpShapeGetElasticity(this->_cp_shape));
        cpShapeSetFriction(new_cp_shape, cpShapeGetFriction(this->_cp_shape));
        cpShapeSetSurfaceVelocity(
            new_cp_shape, cpShapeGetSurfaceVelocity(this->_cp_shape));

        space_safe_call(
            this->space(),
            [shape_ptr = this->_cp_shape](const SpaceNode* space_node_phys) {
                log<LogLevel::debug>(
                    "Simulation callback: destroying old cpShape %p",
                    shape_ptr);
                if (space_node_phys) {
                    cpSpaceRemoveShape(space_node_phys->_cp_space, shape_ptr);
                }
                cpShapeFree(shape_ptr);
            });
        this->_cp_shape = nullptr;
    } else {
        cpShapeSetElasticity(new_cp_shape, 0.95); // default elasticity value
    }

    this->_cp_shape = new_cp_shape;

    if (node->_parent) {
        this->attach_to_simulation();
    }
}

void
HitboxNode::attach_to_simulation()
{
    KAACORE_ASSERT(
        this->_cp_shape != nullptr, "Invalid internal state of hitbox.");
    Node* node = container_node(this);
    if (cpShapeGetBody(this->_cp_shape) == nullptr) {
        log<LogLevel::debug>(
            "Attaching hitbox node %p to simulation (body) (cpShape: %p)", node,
            this->_cp_shape);
        KAACORE_ASSERT(
            node->_parent != nullptr, "Hitbox must to have a parent in order "
                                      "to attach it to the simulation.");
        ASSERT_VALID_BODY_NODE(static_cast<BodyNode*>(&node->_parent->body));
        space_safe_call(
            node->_parent->body.space(),
            [body_ptr = node->_parent->body._cp_body,
             shape_ptr = this->_cp_shape](const SpaceNode* space_node_phys) {
                log<LogLevel::debug>(
                    "Simulation callback: attaching cpShape %p to body "
                    "(cpBody: %p)",
                    shape_ptr, body_ptr);
                cpShapeSetBody(shape_ptr, body_ptr);
            });
    }

    if (cpShapeGetSpace(this->_cp_shape) == nullptr and
        node->_parent->_parent != nullptr) {
        log<LogLevel::debug>(
            "Attaching hitbox node %p to simulation (space) (cpShape: %p)",
            node, this->_cp_shape);
        ASSERT_VALID_SPACE_NODE(
            static_cast<SpaceNode*>(&node->_parent->_parent->space));
        space_safe_call(
            node->_parent->_parent,
            [shape_ptr = this->_cp_shape](const SpaceNode* space_node_phys) {
                log<LogLevel::debug>(
                    "Simulation callback: attaching cpShape %p to space "
                    "(cpSpace: %p)",
                    shape_ptr, space_node_phys->_cp_space);
                cpSpaceAddShape(space_node_phys->_cp_space, shape_ptr);
            });
    }
}

void
HitboxNode::detach_from_simulation()
{
    if (this->_cp_shape != nullptr) {
        log<LogLevel::debug>(
            "Destroying hitbox node %p (cpShape: %p)", container_node(this),
            this->_cp_shape);
        cpShapeSetUserData(this->_cp_shape, nullptr);
        space_safe_call(
            this->space(),
            [shape_ptr = this->_cp_shape](const SpaceNode* space_node_phys) {
                log<LogLevel::debug>(
                    "Simulation callback: destroying cpShape %p", shape_ptr);
                if (space_node_phys) {
                    cpSpaceRemoveShape(space_node_phys->_cp_space, shape_ptr);
                }
                cpShapeFree(shape_ptr);
            });
        this->_cp_shape = nullptr;
    }
}

CollisionTriggerId
HitboxNode::trigger_id()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return static_cast<CollisionTriggerId>(
        cpShapeGetCollisionType(this->_cp_shape));
}

void
HitboxNode::trigger_id(const CollisionTriggerId& trigger_id)
{
    ASSERT_VALID_HITBOX_NODE(this);
    cpShapeSetCollisionType(
        this->_cp_shape, static_cast<cpCollisionType>(trigger_id));
}

CollisionGroup
HitboxNode::group()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return cpShapeGetFilter(this->_cp_shape).group;
}

void
HitboxNode::group(const CollisionGroup& group)
{
    ASSERT_VALID_HITBOX_NODE(this);
    auto filter = cpShapeGetFilter(this->_cp_shape);
    filter.group = group;
    cpShapeSetFilter(this->_cp_shape, filter);
}

CollisionBitmask
HitboxNode::mask()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return cpShapeGetFilter(this->_cp_shape).categories;
}

void
HitboxNode::mask(const CollisionBitmask& mask)
{
    ASSERT_VALID_HITBOX_NODE(this);
    auto filter = cpShapeGetFilter(this->_cp_shape);
    filter.categories = mask;
    cpShapeSetFilter(this->_cp_shape, filter);
}

CollisionBitmask
HitboxNode::collision_mask()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return cpShapeGetFilter(this->_cp_shape).mask;
}

void
HitboxNode::collision_mask(const CollisionBitmask& mask)
{
    ASSERT_VALID_HITBOX_NODE(this);
    auto filter = cpShapeGetFilter(this->_cp_shape);
    filter.mask = mask;
    cpShapeSetFilter(this->_cp_shape, filter);
}

void
HitboxNode::sensor(const bool sensor)
{
    ASSERT_VALID_HITBOX_NODE(this);
    cpShapeSetSensor(this->_cp_shape, sensor);
}

bool
HitboxNode::sensor()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return cpShapeGetSensor(this->_cp_shape);
}

void
HitboxNode::elasticity(const double elasticity)
{
    ASSERT_VALID_HITBOX_NODE(this);
    cpShapeSetElasticity(this->_cp_shape, elasticity);
}

double
HitboxNode::elasticity()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return cpShapeGetElasticity(this->_cp_shape);
}

void
HitboxNode::friction(const double friction)
{
    ASSERT_VALID_HITBOX_NODE(this);
    cpShapeSetFriction(this->_cp_shape, friction);
}

double
HitboxNode::friction()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return cpShapeGetFriction(this->_cp_shape);
}

void
HitboxNode::surface_velocity(const glm::dvec2 surface_velocity)
{
    ASSERT_VALID_HITBOX_NODE(this);
    cpShapeSetSurfaceVelocity(
        this->_cp_shape, convert_vector(surface_velocity));
}

glm::dvec2
HitboxNode::surface_velocity()
{
    ASSERT_VALID_HITBOX_NODE(this);
    return convert_vector(cpShapeGetSurfaceVelocity(this->_cp_shape));
}

} // namespace kaacore
