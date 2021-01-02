#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "kaacore/clock.h"
#include "kaacore/easings.h"
#include "kaacore/node_ptr.h"
#include "kaacore/nodes.h"
#include "kaacore/transitions.h"

namespace kaacore {

enum struct AttributeTransitionMethod {
    set = 1,
    add = 2,
    multiply = 3,
};

template<typename T, typename N, N Node::*N_member, T (N::*F_getter)()>
T
get_node_property(NodePtr node)
{
    if constexpr (std::is_same_v<N, Node>) {
        static_assert(N_member == nullptr);
        return ((node.get())->*F_getter)();
    } else {
        static_assert(N_member != nullptr);
        N* spec_node = &((node.get())->*N_member);
        return (spec_node->*F_getter)();
    }
}

template<
    typename T, typename N, N Node::*N_member, void (N::*F_setter)(const T&)>
void
set_node_property(NodePtr node, const T value)
{
    if constexpr (std::is_same_v<N, Node>) {
        static_assert(N_member == nullptr);
        ((node.get())->*F_setter)(value);
    } else {
        static_assert(N_member != nullptr);
        N* spec_node = &((node.get())->*N_member);
        (spec_node->*F_setter)(value);
    }
}

inline int32_t
find_matching_step(const int32_t steps_count, const double t)
{
    KAACORE_ASSERT(
        0. <= t and t <= 1., "t must be in range [0, 1], was: {}", t);
    return std::min<int32_t>(steps_count * t, steps_count - 1);
}

template<typename T>
inline T
calculate_attribute_advancement(
    const T origin_value, const T advance_value,
    const AttributeTransitionMethod method)
{
    switch (method) {
        case AttributeTransitionMethod::set:
            return advance_value;
        case AttributeTransitionMethod::add:
            return origin_value + advance_value;
        case AttributeTransitionMethod::multiply:
            return origin_value * advance_value;
        default:
            throw kaacore::exception("Unknown transition method.");
    }
}

template<typename T>
struct NodeAttributeTransitionState : TransitionStateBase {
    T origin_value;
    T destination_value;

    NodeAttributeTransitionState(
        T origin, T value_advance, AttributeTransitionMethod advance_method)
        : origin_value(origin),
          destination_value(calculate_attribute_advancement(
              origin, value_advance, advance_method))
    {}
};

template<
    typename T, typename N, N Node::*N_member, T (N::*F_getter)(),
    void (N::*F_setter)(const T&)>
class NodeAttributeTransition : public NodeTransitionCustomizable {
    AttributeTransitionMethod _advance_method;
    T _value_advance;

  public:
    NodeAttributeTransition(
        T value_advance, const AttributeTransitionMethod& advance_method,
        const Duration duration,
        const TransitionWarping& warping = TransitionWarping(),
        const Easing easing = Easing::none)
        : NodeTransitionCustomizable(duration, warping, easing),
          _value_advance(value_advance), _advance_method(advance_method)
    {}

    NodeAttributeTransition(
        T value_advance, const Duration duration,
        const TransitionWarping& warping = TransitionWarping(),
        const Easing easing = Easing::none)
        : NodeAttributeTransition(
              value_advance, AttributeTransitionMethod::set, duration, warping,
              easing)
    {}

    std::unique_ptr<TransitionStateBase> prepare_state(NodePtr node) const
    {
        return std::make_unique<NodeAttributeTransitionState<T>>(
            get_node_property<T, N, N_member, F_getter>(node),
            this->_value_advance, this->_advance_method);
    }

    void evaluate(
        TransitionStateBase* state_b, NodePtr node, const double t) const
    {
        auto state = static_cast<NodeAttributeTransitionState<T>*>(state_b);
        T new_value =
            glm::mix(state->origin_value, state->destination_value, t);
        set_node_property<T, N, N_member, F_setter>(node, new_value);
    }
};

typedef NodeAttributeTransition<
    glm::dvec2, Node, nullptr, &Node::position, &Node::position>
    NodePositionTransition;

typedef NodeAttributeTransition<
    double, Node, nullptr, &Node::rotation, &Node::rotation>
    NodeRotationTransition;

typedef NodeAttributeTransition<
    glm::dvec2, Node, nullptr, &Node::scale, &Node::scale>
    NodeScaleTransition;

typedef NodeAttributeTransition<
    glm::dvec4, Node, nullptr, &Node::color, &Node::color>
    NodeColorTransition;

typedef NodeAttributeTransition<
    glm::dvec2, BodyNode, &Node::body, &BodyNode::velocity, &BodyNode::velocity>
    BodyNodeVelocityTransition;

typedef NodeAttributeTransition<
    double, BodyNode, &Node::body, &BodyNode::angular_velocity,
    &BodyNode::angular_velocity>
    BodyNodeAngularVelocityTransition;

template<typename T>
struct NodeAttributeSteppingTransitionState : TransitionStateBase {
    T origin_value;
    int32_t last_step_index = -1;

    NodeAttributeSteppingTransitionState(T origin_value)
        : origin_value(origin_value)
    {}
};

template<
    typename T, typename N, N Node::*N_member, T (N::*F_getter)(),
    void (N::*F_setter)(const T&)>
class NodeAttributeSteppingTransition : public NodeTransitionCustomizable {
    AttributeTransitionMethod _advance_method;
    std::vector<T> _steps;

  public:
    NodeAttributeSteppingTransition(
        const std::vector<T>& steps,
        const AttributeTransitionMethod& advance_method,
        const Duration duration,
        const TransitionWarping& warping = TransitionWarping(),
        const Easing easing = Easing::none)
        : NodeTransitionCustomizable(duration, warping, easing), _steps(steps),
          _advance_method(advance_method)
    {}

    NodeAttributeSteppingTransition(
        const std::vector<T>& steps, const Duration duration,
        const TransitionWarping& warping = TransitionWarping(),
        const Easing easing = Easing::none)
        : NodeAttributeSteppingTransition(
              steps, AttributeTransitionMethod::set, duration, warping, easing)
    {}

    std::unique_ptr<TransitionStateBase> prepare_state(NodePtr node) const
    {
        return std::make_unique<NodeAttributeSteppingTransitionState<T>>(
            get_node_property<T, N, N_member, F_getter>(node));
    }

    void evaluate(
        TransitionStateBase* state_b, NodePtr node, const double t) const
    {
        auto state =
            static_cast<NodeAttributeSteppingTransitionState<T>*>(state_b);
        auto target_step = find_matching_step(this->_steps.size(), t);
        KAACORE_LOG_TRACE(
            "NodeAttributeSteppingTransition({})::evaluate - node: {}, t: "
            "{}, steps_count: {}, target_step {}",
            fmt::ptr(this), fmt::ptr(node.get()), t, this->_steps.size(),
            target_step);

        if (target_step != state->last_step_index) {
            state->last_step_index = target_step;
            const auto advance_value = this->_steps[target_step];
            set_node_property<T, N, N_member, F_setter>(
                node,
                calculate_attribute_advancement(
                    state->origin_value, advance_value, this->_advance_method));
        };
    }
};

typedef NodeAttributeSteppingTransition<
    glm::dvec2, Node, nullptr, &Node::position, &Node::position>
    NodePositionSteppingTransition;

typedef NodeAttributeSteppingTransition<
    double, Node, nullptr, &Node::rotation, &Node::rotation>
    NodeRotationSteppingTransition;

typedef NodeAttributeSteppingTransition<
    glm::dvec2, Node, nullptr, &Node::scale, &Node::scale>
    NodeScaleSteppingTransition;

typedef NodeAttributeSteppingTransition<
    glm::dvec4, Node, nullptr, &Node::color, &Node::color>
    NodeColorSteppingTransition;

typedef NodeAttributeSteppingTransition<
    glm::dvec2, BodyNode, &Node::body, &BodyNode::velocity, &BodyNode::velocity>
    BodyNodeVelocitySteppingTransition;

typedef NodeAttributeSteppingTransition<
    double, BodyNode, &Node::body, &BodyNode::angular_velocity,
    &BodyNode::angular_velocity>
    BodyNodeAngularVelocitySteppingTransition;

struct _NodeSteppingTransitionBasicState : TransitionStateBase {
    int32_t last_step_index = -1;
};

template<
    typename T, typename N, N Node::*N_member, void (N::*F_setter)(const T&)>
class NodeInoperableAttributeSteppingTransition
    : public NodeTransitionCustomizable {
    std::vector<T> _steps;

  public:
    NodeInoperableAttributeSteppingTransition(
        const std::vector<T>& steps, const Duration duration,
        const TransitionWarping& warping = TransitionWarping(),
        const Easing easing = Easing::none)
        : NodeTransitionCustomizable(duration, warping, easing), _steps(steps)
    {
        KAACORE_CHECK(
            this->_steps.size() > 0,
            "Number of steps must be greater than zero.");
    }

    std::unique_ptr<TransitionStateBase> prepare_state(NodePtr node) const
    {
        return std::make_unique<_NodeSteppingTransitionBasicState>();
    }

    void evaluate(
        TransitionStateBase* state_b, NodePtr node, const double t) const
    {
        auto state = static_cast<_NodeSteppingTransitionBasicState*>(state_b);
        auto target_step = find_matching_step(this->_steps.size(), t);
        KAACORE_LOG_TRACE(
            "NodeInoperableAttributeSteppingTransition({})::evaluate - node: "
            "{}, t: "
            "{}, steps_count: {}, target_step {}",
            fmt::ptr(this), fmt::ptr(node.get()), t, this->_steps.size(),
            target_step);

        if (target_step != state->last_step_index) {
            state->last_step_index = target_step;
            set_node_property<T, N, N_member, F_setter>(
                node, this->_steps[target_step]);
        };
    }
};

typedef NodeInoperableAttributeSteppingTransition<
    Sprite, Node, nullptr, &Node::sprite>
    NodeSpriteTransition;

typedef NodeInoperableAttributeSteppingTransition<
    Shape, Node, nullptr, &Node::shape>
    NodeShapeSteppingTransition;

typedef NodeInoperableAttributeSteppingTransition<
    std::optional<int16_t>, Node, nullptr, &Node::z_index>
    NodeZIndexSteppingTransition;

} // namespace kaacore
