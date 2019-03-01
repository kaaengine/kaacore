#include <type_traits>

#include <glm/gtc/matrix_transform.hpp>
#include <chipmunk/chipmunk.h>

#include "kaacore/nodes.h"
#include "kaacore/utils.h"
#include "kaacore/log.h"

#include "kaacore/physics.h"


namespace kaacore {

// assertion helpers

#define ASSERT_VALID_SPACE_NODE() \
    assert(container_node(this)->type == NodeType::space); \
    assert(this->cp_space != nullptr);

#define ASSERT_VALID_BODY_NODE() \
    assert(container_node(this)->type == NodeType::body); \
    assert(this->cp_body != nullptr);

#define ASSERT_DYNAMIC_BODY_NODE() \
    ASSERT_VALID_BODY_NODE(); \
    assert(this->get_body_type() == BodyNodeType::dynamic);

#define ASSERT_VALID_HITBOX_NODE() \
    assert(container_node(this)->type == NodeType::hitbox); \
    assert(this->cp_shape != nullptr);


// conversions

cpVect convert_vector(const glm::dvec2 vec)
{
    return {vec.x, vec.y};
}

glm::dvec2 convert_vector(const cpVect vec)
{
    return {vec.x, vec.y};
}


inline constexpr Node* container_node(const SpaceNode* space)
{
    // static_assert(std::is_standard_layout<Node>::value, "");
    return container_of(space, &Node::space);
}


inline constexpr Node* container_node(const BodyNode* body)
{
    // static_assert(std::is_standard_layout<Node>::value, "");
    return container_of(body, &Node::body);
}


inline constexpr Node* container_node(const HitboxNode* hitbox)
{
    // static_assert(std::is_standard_layout<Node>::value, "");
    return container_of(hitbox, &Node::hitbox);
}


Arbiter::Arbiter(cpArbiter* arbiter)
    : cp_arbiter(arbiter)
{
}


CollisionPair::CollisionPair(BodyNode* body, HitboxNode* hitbox)
    : body(container_node(body)), hitbox(container_node(hitbox))
{
}


uint8_t operator|(CollisionPhase phase, uint8_t other)
{
    return uint8_t(phase) | other;
}

uint8_t operator|(CollisionPhase phase, CollisionPhase other)
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


void SpaceNode::initialize()
{
    log("Creating simulation node: %p", container_node(this));
    this->cp_space = cpSpaceNew();
    cpSpaceSetUserData(this->cp_space, container_node(this));
    this->time_acc = 0;
}

void SpaceNode::destroy()
{
    cpSpaceDestroy(this->cp_space);
    // TODO destroy collision handlers?
}

void SpaceNode::simulate(const uint32_t dt)
{
    ASSERT_VALID_SPACE_NODE();
    uint32_t time_left = dt + this->time_acc;
    while (time_left > default_simulation_step_size) {
        cpSpaceStep(this->cp_space, 0.001 * default_simulation_step_size);
        time_left -= default_simulation_step_size;
    }
    this->time_acc = time_left;
}

template<typename R_type, CollisionPhase phase, bool non_null_nodes>
R_type _chipmunk_collision_handler(cpArbiter* cp_arbiter, cpSpace* cp_space,
                                 cpDataPointer data)
{
    auto func = static_cast<CollisionHandlerFunc*>(data);

    cpBody* cp_body_a = nullptr;
    cpBody* cp_body_b = nullptr;
    cpShape* cp_shape_a = nullptr;
    cpShape* cp_shape_b = nullptr;
    cpArbiterGetBodies(cp_arbiter, &cp_body_a, &cp_body_b);
    cpArbiterGetShapes(cp_arbiter, &cp_shape_a, &cp_shape_b);

    assert(cp_body_a != nullptr);
    assert(cp_body_b != nullptr);
    assert(cp_shape_a != nullptr);
    assert(cp_shape_b != nullptr);

    auto body_a = static_cast<BodyNode*>(cpBodyGetUserData(cp_body_a));
    auto body_b = static_cast<BodyNode*>(cpBodyGetUserData(cp_body_b));
    auto hitbox_a = static_cast<HitboxNode*>(cpShapeGetUserData(cp_shape_a));
    auto hitbox_b = static_cast<HitboxNode*>(cpShapeGetUserData(cp_shape_b));

    if (non_null_nodes and (
            body_a == nullptr or body_b == nullptr or
            hitbox_a == nullptr or hitbox_b == nullptr
    )) {
        return R_type(0);
    }

    return (R_type)(*func)(
        phase,
        Arbiter(cp_arbiter),
        CollisionPair(body_a, hitbox_a),
        CollisionPair(body_b, hitbox_b)
    );
}

template<typename R_type>
R_type _chipmunk_collision_noop(cpArbiter* cp_arbiter, cpSpace* cp_space,
                                cpDataPointer data)
{
    return R_type(1);
}


void SpaceNode::set_collision_handler(
    CollisionTriggerId trigger_a, CollisionTriggerId trigger_b,
    CollisionHandlerFunc handler, uint8_t phases_mask,
    bool only_non_deleted_nodes
)
{
    cpCollisionHandler* cp_handler = cpSpaceAddCollisionHandler(
        this->cp_space, static_cast<cpCollisionType>(trigger_a),
        static_cast<cpCollisionType>(trigger_b)
    );


    if (cp_handler->userData != nullptr) {
        // TODO handle existing
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
            cp_handler->beginFunc = \
                &_chipmunk_collision_handler<uint8_t, CollisionPhase::begin, true>;
        }
        if (CollisionPhase::pre_solve & phases_mask) {
            cp_handler->preSolveFunc = \
                &_chipmunk_collision_handler<uint8_t, CollisionPhase::pre_solve, true>;
        }
        if (CollisionPhase::post_solve & phases_mask) {
            cp_handler->postSolveFunc = \
                &_chipmunk_collision_handler<void, CollisionPhase::post_solve, true>;
        }
        if (CollisionPhase::separate & phases_mask) {
            cp_handler->separateFunc = \
                &_chipmunk_collision_handler<void, CollisionPhase::separate, true>;
        }
    } else {
        if (CollisionPhase::begin & phases_mask) {
            cp_handler->beginFunc = \
                &_chipmunk_collision_handler<uint8_t, CollisionPhase::begin, false>;
        }
        if (CollisionPhase::pre_solve & phases_mask) {
            cp_handler->preSolveFunc = \
                &_chipmunk_collision_handler<uint8_t, CollisionPhase::pre_solve, false>;
        }
        if (CollisionPhase::post_solve & phases_mask) {
            cp_handler->postSolveFunc = \
                &_chipmunk_collision_handler<void, CollisionPhase::post_solve, false>;
        }
        if (CollisionPhase::separate & phases_mask) {
            cp_handler->separateFunc = \
                &_chipmunk_collision_handler<void, CollisionPhase::separate, false>;
        }
    }
}

void SpaceNode::set_gravity(const glm::dvec2 gravity)
{
    ASSERT_VALID_SPACE_NODE();
    cpSpaceSetGravity(this->cp_space, convert_vector(gravity));
}

glm::dvec2 SpaceNode::get_gravity() const
{
    ASSERT_VALID_SPACE_NODE();
    return convert_vector(cpSpaceGetGravity(this->cp_space));
}

void SpaceNode::set_damping(const double damping)
{
    ASSERT_VALID_SPACE_NODE();
    cpSpaceSetDamping(this->cp_space, damping);
}

double SpaceNode::get_damping() const
{
    ASSERT_VALID_SPACE_NODE();
    return cpSpaceGetDamping(this->cp_space);
}

void SpaceNode::set_sleeping_threshold(const double threshold)
{
    ASSERT_VALID_SPACE_NODE();
    cpSpaceSetSleepTimeThreshold(this->cp_space, threshold);
}

double SpaceNode::get_sleeping_threshold() const
{
    ASSERT_VALID_SPACE_NODE();
    return cpSpaceGetSleepTimeThreshold(this->cp_space);
}

bool SpaceNode::is_locked() const
{
    ASSERT_VALID_SPACE_NODE();
    return cpSpaceIsLocked(this->cp_space);
}


void BodyNode::initialize()
{
    this->cp_body = cpBodyNewKinematic();
    cpBodySetUserData(this->cp_body, container_node(this));
}

void BodyNode::destroy()
{
    if (this->cp_body != nullptr) {
        // TODO destroy body or delegate after cpSpace step
    }
}

void BodyNode::attach_to_simulation()
{
    ASSERT_VALID_BODY_NODE();

    if (cpBodyGetSpace(this->cp_body) == nullptr) {
        Node* node = container_node(this);
        log("Attaching body node %p to simulation (space)", node);
        assert(node->parent != nullptr);
        assert(node->parent->type == NodeType::space);
        assert(node->parent->space.cp_space != nullptr);
        cpSpaceAddBody(node->parent->space.cp_space, this->cp_body);
    }
}

void BodyNode::set_body_type(const BodyNodeType type)
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetType(this->cp_body, static_cast<cpBodyType>(type));

    if (type == BodyNodeType::dynamic) {
        if (this->get_mass() == 0.) {
            this->set_mass(20.);
        }
        if (this->get_moment() == 0.) {
            this->set_moment(10000.);
        }
    }
}
BodyNodeType BodyNode::get_body_type() const
{
    ASSERT_VALID_BODY_NODE();
    return static_cast<BodyNodeType>(cpBodyGetType(this->cp_body));
}

void BodyNode::set_mass(const double m)
{
    ASSERT_DYNAMIC_BODY_NODE();
    cpBodySetMass(this->cp_body, m);
}

double BodyNode::get_mass() const
{
    ASSERT_DYNAMIC_BODY_NODE();
    return cpBodyGetMass(this->cp_body);
}

void BodyNode::set_moment(const double i)
{
    ASSERT_DYNAMIC_BODY_NODE();
    cpBodySetMoment(this->cp_body, i);
}

double BodyNode::get_moment() const
{
    ASSERT_DYNAMIC_BODY_NODE();
    return cpBodyGetMoment(this->cp_body);
}

void BodyNode::override_simulation_position()
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetPosition(this->cp_body,
                      convert_vector(container_node(this)->position));
}

void BodyNode::sync_simulation_position() const
{
    ASSERT_VALID_BODY_NODE();
    container_node(this)->position = \
        convert_vector(cpBodyGetPosition(this->cp_body));
}


void BodyNode::override_simulation_rotation()
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetAngle(this->cp_body, container_node(this)->rotation);
}

void BodyNode::sync_simulation_rotation() const
{
    ASSERT_VALID_BODY_NODE();
    container_node(this)->rotation = cpBodyGetAngle(this->cp_body);
}

void BodyNode::set_velocity(const glm::dvec2 velocity)
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetVelocity(this->cp_body, convert_vector(velocity));
}

glm::dvec2 BodyNode::get_velocity() const
{
    ASSERT_VALID_BODY_NODE();
    return convert_vector(cpBodyGetVelocity(this->cp_body));
}

void BodyNode::set_force(const glm::dvec2 force)
{
    cpBodySetForce(this->cp_body, convert_vector(force));
}

glm::dvec2 BodyNode::get_force() const
{
    ASSERT_VALID_BODY_NODE();
    return convert_vector(cpBodyGetForce(this->cp_body));
}

void BodyNode::set_torque(const double torque)
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetTorque(this->cp_body, torque);
}

double BodyNode::get_torque() const
{
    ASSERT_VALID_BODY_NODE();
    return cpBodyGetTorque(this->cp_body);
}

void BodyNode::set_angular_velocity(const double angular_velocity)
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetAngularVelocity(this->cp_body, angular_velocity);
}

double BodyNode::get_angular_velocity() const
{
    ASSERT_VALID_BODY_NODE();
    return cpBodyGetAngularVelocity(this->cp_body);
}

bool BodyNode::is_sleeping() const
{
    ASSERT_VALID_BODY_NODE();
    return cpBodyIsSleeping(this->cp_body);
}

void BodyNode::sleep()
{
    ASSERT_VALID_BODY_NODE();
    cpBodySleep(this->cp_body);
}

void BodyNode::activate()
{
    ASSERT_VALID_BODY_NODE();
    cpBodyActivate(this->cp_body);
}

void HitboxNode::initialize()
{
}


void HitboxNode::destroy()
{
    if (this->cp_shape != nullptr) {
        // TODO destroy shape or delegate after cpSpace step
    }
}

void HitboxNode::update_physics_shape()
{
    if (this->cp_shape != nullptr) {
        // TODO destroy old shape or delegate after cpSpace step
    }

    Node* node = container_node(this);
    log("Updating hitbox node %p shape", node);

    assert(node->shape.type != ShapeType::none);
    assert(node->shape.type != ShapeType::freeform);
    cpVect* cp_points = reinterpret_cast<cpVect*>(node->shape.points.data());
    // TODO handle node matrix transformations
    if (node->shape.type == ShapeType::segment) {
        assert(node->shape.points.size() == 2);
        this->cp_shape = cpSegmentShapeNew(
            nullptr, cp_points[0], cp_points[1], node->shape.radius
        );
    } else if (node->shape.type == ShapeType::circle) {
        assert(node->shape.points.size() == 1);
        this->cp_shape = cpCircleShapeNew(
            nullptr, node->shape.radius, cp_points[0]
        );
    } else if (node->shape.type == ShapeType::polygon) {
        this->cp_shape = cpPolyShapeNew(
            nullptr, node->shape.points.size(), cp_points,
            cpTransformIdentity, 0.
        );
    }

    cpShapeSetUserData(this->cp_shape, node);

    // XXX default elasticity?
    cpShapeSetElasticity(this->cp_shape, 0.95);
}

void HitboxNode::attach_to_simulation()
{
    assert(this->cp_shape != nullptr);
    Node* node = container_node(this);

    if (cpShapeGetBody(this->cp_shape) == nullptr) {
        log("Attaching hitbox node %p to simulation (body)", node);
        assert(node->parent != nullptr);
        assert(node->parent->type == NodeType::body);
        assert(node->parent->body.cp_body != nullptr);
        cpShapeSetBody(this->cp_shape, node->parent->body.cp_body);
    }

    if (cpShapeGetSpace(this->cp_shape) == nullptr
        and node->parent->parent != nullptr) {
        log("Attaching hitbox node %p to simulation (space)", node);
        assert(node->parent->parent->type == NodeType::space);
        assert(node->parent->parent->space.cp_space != nullptr);
        cpSpaceAddShape(node->parent->parent->space.cp_space, this->cp_shape);
    }
}

void HitboxNode::set_trigger_id(const CollisionTriggerId trigger_id)
{
    ASSERT_VALID_HITBOX_NODE();
    cpShapeSetCollisionType(this->cp_shape, static_cast<cpCollisionType>(trigger_id));
}

CollisionTriggerId HitboxNode::get_trigger_id() const
{
    ASSERT_VALID_HITBOX_NODE();
    return static_cast<CollisionTriggerId>(cpShapeGetCollisionType(this->cp_shape));
}

void HitboxNode::set_group(const CollisionGroup group)
{
    ASSERT_VALID_HITBOX_NODE();
    auto filter = cpShapeGetFilter(this->cp_shape);
    filter.group = group;
    cpShapeSetFilter(this->cp_shape, filter);
}

CollisionGroup HitboxNode::get_group() const
{
    ASSERT_VALID_HITBOX_NODE();
    return cpShapeGetFilter(this->cp_shape).group;
}


void HitboxNode::set_mask(const CollisionBitmask mask)
{
    ASSERT_VALID_HITBOX_NODE();
    auto filter = cpShapeGetFilter(this->cp_shape);
    filter.categories = mask;
    cpShapeSetFilter(this->cp_shape, filter);
}

CollisionBitmask HitboxNode::get_mask() const
{
    ASSERT_VALID_HITBOX_NODE();
    return cpShapeGetFilter(this->cp_shape).categories;
}


void HitboxNode::set_collision_mask(const CollisionBitmask mask)
{
    ASSERT_VALID_HITBOX_NODE();
    auto filter = cpShapeGetFilter(this->cp_shape);
    filter.mask = mask;
    cpShapeSetFilter(this->cp_shape, filter);
}

CollisionBitmask HitboxNode::get_collision_mask() const
{
    ASSERT_VALID_HITBOX_NODE();
    return cpShapeGetFilter(this->cp_shape).mask;
}

} // namespace kaacore
