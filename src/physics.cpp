#include <type_traits>

#include <glm/gtc/matrix_transform.hpp>
#include <chipmunk/chipmunk.h>

#include "kaacore/nodes.h"
#include "kaacore/utils.h"
#include "kaacore/log.h"

#include "kaacore/physics.h"


// assertion helpers

#define ASSERT_VALID_BODY_NODE() \
    assert(container_node(this)->type == NodeType::body); \
    assert(this->cp_body != nullptr);

#define ASSERT_DYNAMIC_BODY_NODE() \
    ASSERT_VALID_BODY_NODE(); \
    assert(this->get_body_type() == BodyNodeType::dynamic);


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


void SpaceNode::initialize()
{
    log("Creating simulation node: %p", container_node(this));
    this->cp_space = cpSpaceNew();
    cpSpaceSetUserData(this->cp_space, container_node(this));
}

void SpaceNode::destroy()
{
    cpSpaceDestroy(this->cp_space);
    // TODO destroy collision handlers?
}

void SpaceNode::simulate(uint32_t dt)
{
    cpSpaceStep(this->cp_space, 1.);
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
            this->set_mass(10.);
        }
        if (this->get_moment() == 0.) {
            this->set_moment(1000.);
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

void BodyNode::override_simulation_position()
{
    ASSERT_VALID_BODY_NODE();
    cpBodySetPosition(this->cp_body,
                      convert_vector(container_node(this)->position));
}

void BodyNode::sync_simulation_position()
{
    ASSERT_VALID_BODY_NODE();
    container_node(this)->position = \
        convert_vector(cpBodyGetPosition(this->cp_body));
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
