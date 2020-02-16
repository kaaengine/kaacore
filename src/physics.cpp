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
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/utils.h"

#include "kaacore/physics.h"

namespace kaacore {

// assertion helpers

#define ASSERT_VALID_SPACE_NODE()                                              \
    KAACORE_ASSERT(container_node(this)->_type == NodeType::space);            \
    KAACORE_ASSERT(this->_cp_space != nullptr);

#define ASSERT_VALID_BODY_NODE()                                               \
    KAACORE_ASSERT(container_node(this)->_type == NodeType::body);             \
    KAACORE_ASSERT(this->_cp_body != nullptr);

#define ASSERT_DYNAMIC_BODY_NODE()                                             \
    ASSERT_VALID_BODY_NODE();                                                  \
    KAACORE_ASSERT(this->body_type() == BodyNodeType::dynamic);

#define ASSERT_VALID_HITBOX_NODE()                                             \
    KAACORE_ASSERT(container_node(this)->_type == NodeType::hitbox);           \
    KAACORE_ASSERT(this->_cp_shape != nullptr);

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

struct HitboxTransform {
    glm::dvec2 translation;
    double rotation;
    glm::dvec2 scale;

    cpTransform _transform;

    HitboxTransform(const glm::dvec2& tr, const double& r, const glm::dvec2& sc)
        : translation(tr), scale(sc), rotation(r)
    {
        this->_transform = cpTransformMult(
            cpTransformTranslate(convert_vector(this->translation)),
            cpTransformMult(
                cpTransformRotate(this->rotation),
                cpTransformScale(this->scale.x, this->scale.y)));
    }

    std::vector<cpVect> transform_points(
        const std::vector<glm::dvec2>& points) const
    {
        std::vector<cpVect> cp_points;
        cp_points.reserve(points.size());

        for (const auto& pt : points) {
            cp_points.emplace_back(
                cpTransformPoint(this->_transform, convert_vector(pt)));
        }

        return cp_points;
    }

    double flat_radius() const
    {
        KAACORE_CHECK(this->scale.x == this->scale.y);
        return this->scale.x;
    }
};

inline constexpr Node*
container_node(const SpaceNode* space)
{
    // static_assert(std::is_standard_layout<Node>::value, "");
    return container_of(space, &Node::space);
}

inline constexpr Node*
container_node(const BodyNode* body)
{
    // static_assert(std::is_standard_layout<Node>::value, "");
    return container_of(body, &Node::body);
}

inline constexpr Node*
container_node(const HitboxNode* hitbox)
{
    // static_assert(std::is_standard_layout<Node>::value, "");
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
    KAACORE_ASSERT(space_node->type() == NodeType::space);
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
    KAACORE_ASSERT_TERMINATE(cp_collision_handler->userData != nullptr);
    auto collision_handler_func =
        static_cast<CollisionHandlerFunc*>(cp_collision_handler->userData);
    delete collision_handler_func;
}

SpaceNode::~SpaceNode()
{
    log<LogLevel::debug>(
        "Destroying space node %p (cpSpace: %p)", container_node(this),
        this->_cp_space);

    KAACORE_ASSERT_TERMINATE(this->_cp_space != nullptr);

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
    ASSERT_VALID_SPACE_NODE();
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

    KAACORE_ASSERT(cp_body_a != nullptr);
    KAACORE_ASSERT(cp_body_b != nullptr);
    KAACORE_ASSERT(cp_shape_a != nullptr);
    KAACORE_ASSERT(cp_shape_b != nullptr);

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

glm::dvec2
SpaceNode::gravity()
{
    ASSERT_VALID_SPACE_NODE();
    return convert_vector(cpSpaceGetGravity(this->_cp_space));
}

void
SpaceNode::gravity(const glm::dvec2& gravity)
{
    ASSERT_VALID_SPACE_NODE();
    cpSpaceSetGravity(this->_cp_space, convert_vector(gravity));
}

double
SpaceNode::damping()
{
    ASSERT_VALID_SPACE_NODE();
    return cpSpaceGetDamping(this->_cp_space);
}

void
SpaceNode::damping(const double& damping)
{
    ASSERT_VALID_SPACE_NODE();
    cpSpaceSetDamping(this->_cp_space, damping);
}

double
SpaceNode::sleeping_threshold()
{
    ASSERT_VALID_SPACE_NODE();
    return cpSpaceGetSleepTimeThreshold(this->_cp_space);
}

void
SpaceNode::sleeping_threshold(const double& threshold)
{
    ASSERT_VALID_SPACE_NODE();
    cpSpaceSetSleepTimeThreshold(this->_cp_space, threshold);
}

bool
SpaceNode::locked() const
{
    ASSERT_VALID_SPACE_NODE();
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
BodyNode::attach_to_simulation()
{
    ASSERT_VALID_BODY_NODE();

    if (cpBodyGetSpace(this->_cp_body) == nullptr) {
        Node* node = container_node(this);
        log<LogLevel::debug>(
            "Attaching body node %p to simulation (space) (cpBody: %p)", node,
            this->_cp_body);
        KAACORE_ASSERT(node->_parent != nullptr);
        KAACORE_ASSERT(node->_parent->_type == NodeType::space);
        KAACORE_ASSERT(node->_parent->space._cp_space != nullptr);
        space_safe_call(node->_parent, [&](const SpaceNode* space_node_phys) {
            log<LogLevel::debug>(
                "Simulation callback: attaching cpBody %p", this->_cp_body);
            cpSpaceAddBody(space_node_phys->_cp_space, this->_cp_body);
        });
    }
}

void
BodyNode::override_simulation_position()
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetPosition(
        this->_cp_body, convert_vector(container_node(this)->_position));
}

void
BodyNode::sync_simulation_position() const
{
    ASSERT_VALID_BODY_NODE();
    container_node(this)->_set_position(
        convert_vector(cpBodyGetPosition(this->_cp_body)));
}

void
BodyNode::override_simulation_rotation()
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetAngle(this->_cp_body, container_node(this)->_rotation);
}

void
BodyNode::sync_simulation_rotation() const
{
    ASSERT_VALID_BODY_NODE();
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
    ASSERT_VALID_BODY_NODE();
    return static_cast<BodyNodeType>(cpBodyGetType(this->_cp_body));
}

void
BodyNode::body_type(const BodyNodeType& type)
{
    ASSERT_VALID_BODY_NODE();
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
    ASSERT_DYNAMIC_BODY_NODE();
    return cpBodyGetMass(this->_cp_body);
}

void
BodyNode::mass(const double& m)
{
    ASSERT_DYNAMIC_BODY_NODE();
    cpBodySetMass(this->_cp_body, m);
}

double
BodyNode::moment()
{
    ASSERT_DYNAMIC_BODY_NODE();
    return cpBodyGetMoment(this->_cp_body);
}

void
BodyNode::moment(const double& i)
{
    ASSERT_DYNAMIC_BODY_NODE();
    cpBodySetMoment(this->_cp_body, i);
}

glm::dvec2
BodyNode::velocity()
{
    ASSERT_VALID_BODY_NODE();
    return convert_vector(cpBodyGetVelocity(this->_cp_body));
}

void
BodyNode::velocity(const glm::dvec2& velocity)
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetVelocity(this->_cp_body, convert_vector(velocity));
}

glm::dvec2
BodyNode::force()
{
    ASSERT_VALID_BODY_NODE();
    return convert_vector(cpBodyGetForce(this->_cp_body));
}

void
BodyNode::force(const glm::dvec2& force)
{
    cpBodySetForce(this->_cp_body, convert_vector(force));
}

double
BodyNode::torque()
{
    ASSERT_VALID_BODY_NODE();
    return cpBodyGetTorque(this->_cp_body);
}

void
BodyNode::torque(const double& torque)
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetTorque(this->_cp_body, torque);
}

double
BodyNode::angular_velocity()
{
    ASSERT_VALID_BODY_NODE();
    return cpBodyGetAngularVelocity(this->_cp_body);
}

void
BodyNode::angular_velocity(const double& angular_velocity)
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetAngularVelocity(this->_cp_body, angular_velocity);
}

bool
BodyNode::sleeping()
{
    ASSERT_VALID_BODY_NODE();
    return cpBodyIsSleeping(this->_cp_body);
}

void
BodyNode::sleeping(const bool& sleeping)
{
    ASSERT_VALID_BODY_NODE();
    if (sleeping) {
        cpBodySleep(this->_cp_body);
    } else {
        cpBodyActivate(this->_cp_body);
    }
}

HitboxNode::HitboxNode() {}

HitboxNode::~HitboxNode()
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

    const HitboxTransform hitbox_transform(
        node->_position, node->_rotation,
        // include parent's (BodyNode) scale
        node->_scale *
            (node->_parent ? node->_parent->_scale : glm::dvec2(1.)));

    KAACORE_ASSERT(node->_shape.type != ShapeType::none);
    KAACORE_ASSERT(node->_shape.type != ShapeType::freeform);
    std::vector<cpVect> cp_points =
        hitbox_transform.transform_points(node->_shape.points);
    if (node->_shape.type == ShapeType::segment) {
        KAACORE_ASSERT(node->_shape.points.size() == 2);
        new_cp_shape = cpSegmentShapeNew(
            nullptr, cp_points[0], cp_points[1],
            node->_shape.radius * hitbox_transform.flat_radius());
    } else if (node->_shape.type == ShapeType::circle) {
        KAACORE_ASSERT(node->_shape.points.size() == 1);
        new_cp_shape = cpCircleShapeNew(
            nullptr, node->_shape.radius * hitbox_transform.flat_radius(),
            cp_points[0]);
    } else if (node->_shape.type == ShapeType::polygon) {
        new_cp_shape = cpPolyShapeNewRaw(
            nullptr, node->_shape.points.size(), cp_points.data(), 0.);
    }

    log<LogLevel::debug>(
        "Updating hitbox node %p shape (cpShape: %p)", node, new_cp_shape);

    cpShapeSetUserData(new_cp_shape, this);
    cpShapeSetElasticity(new_cp_shape, 0.95);

    if (this->_cp_shape != nullptr) {
        cpShapeSetUserData(this->_cp_shape, nullptr);

        // copy over existing cpShape parameters
        cpShapeSetCollisionType(
            new_cp_shape, cpShapeGetCollisionType(this->_cp_shape));
        cpShapeSetFilter(new_cp_shape, cpShapeGetFilter(this->_cp_shape));

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
    }

    this->_cp_shape = new_cp_shape;

    if (node->_parent) {
        this->attach_to_simulation();
    }
}

void
HitboxNode::attach_to_simulation()
{
    KAACORE_ASSERT(this->_cp_shape != nullptr);
    Node* node = container_node(this);

    if (cpShapeGetBody(this->_cp_shape) == nullptr) {
        log<LogLevel::debug>(
            "Attaching hitbox node %p to simulation (body) (cpShape: %p)", node,
            this->_cp_shape);
        KAACORE_ASSERT(node->_parent != nullptr);
        KAACORE_ASSERT(node->_parent->_type == NodeType::body);
        KAACORE_ASSERT(node->_parent->body._cp_body != nullptr);
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
        KAACORE_ASSERT(node->_parent->_parent->_type == NodeType::space);
        KAACORE_ASSERT(node->_parent->_parent->space._cp_space != nullptr);
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

CollisionTriggerId
HitboxNode::trigger_id()
{
    ASSERT_VALID_HITBOX_NODE();
    return static_cast<CollisionTriggerId>(
        cpShapeGetCollisionType(this->_cp_shape));
}

void
HitboxNode::trigger_id(const CollisionTriggerId& trigger_id)
{
    ASSERT_VALID_HITBOX_NODE();
    cpShapeSetCollisionType(
        this->_cp_shape, static_cast<cpCollisionType>(trigger_id));
}

CollisionGroup
HitboxNode::group()
{
    ASSERT_VALID_HITBOX_NODE();
    return cpShapeGetFilter(this->_cp_shape).group;
}

void
HitboxNode::group(const CollisionGroup& group)
{
    ASSERT_VALID_HITBOX_NODE();
    auto filter = cpShapeGetFilter(this->_cp_shape);
    filter.group = group;
    cpShapeSetFilter(this->_cp_shape, filter);
}

CollisionBitmask
HitboxNode::mask()
{
    ASSERT_VALID_HITBOX_NODE();
    return cpShapeGetFilter(this->_cp_shape).categories;
}

void
HitboxNode::mask(const CollisionBitmask& mask)
{
    ASSERT_VALID_HITBOX_NODE();
    auto filter = cpShapeGetFilter(this->_cp_shape);
    filter.categories = mask;
    cpShapeSetFilter(this->_cp_shape, filter);
}

CollisionBitmask
HitboxNode::collision_mask()
{
    ASSERT_VALID_HITBOX_NODE();
    return cpShapeGetFilter(this->_cp_shape).mask;
}

void
HitboxNode::collision_mask(const CollisionBitmask& mask)
{
    ASSERT_VALID_HITBOX_NODE();
    auto filter = cpShapeGetFilter(this->_cp_shape);
    filter.mask = mask;
    cpShapeSetFilter(this->_cp_shape, filter);
}

} // namespace kaacore
