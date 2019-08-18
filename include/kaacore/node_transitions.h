#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "kaacore/transitions.h"
#include "kaacore/nodes.h"


namespace kaacore {

enum struct AttributeTransitionMethod {
    set = 1,
    add = 2,
    multiply = 3,
};


template<typename T>
struct NodeAttributeTransitionState : TransitionStateBase {
    T origin_value;
    T destination_value;

    NodeAttributeTransitionState(T origin, T value_advance, AttributeTransitionMethod advance_method)
    : origin_value(origin)
    {
        if (advance_method == AttributeTransitionMethod::set) {
            this->destination_value = value_advance;
        } else if (advance_method == AttributeTransitionMethod::add) {
            this->destination_value = origin + value_advance;
        } else {
            this->destination_value = origin * value_advance;
        }
    }
};


template<typename T,
         T (Node::*F_getter)(),
         void(Node::*F_setter)(const T&)>
class NodeAttributeTransition : public NodeTransitionBase {
    AttributeTransitionMethod _advance_method;
    T _value_advance;

    public:
    NodeAttributeTransition(T value_advance, const AttributeTransitionMethod& advance_method,
                            const double duration, const TransitionWarping& warping = TransitionWarping())
    : NodeTransitionBase(duration, warping), _value_advance(value_advance), _advance_method(advance_method)
    {
    }

    NodeAttributeTransition(T value_advance, const double duration,
                            const TransitionWarping& warping = TransitionWarping())
    : NodeAttributeTransition(value_advance, AttributeTransitionMethod::set, duration, warping)
    {
    }

    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const
    {
        return std::make_unique<NodeAttributeTransitionState<T>>(
            (node->*F_getter)(), this->_value_advance, this->_advance_method
        );
    }

    void evaluate(TransitionStateBase* state_b, Node* node, const double t) const
    {
        auto state = static_cast<NodeAttributeTransitionState<T>*>(state_b);
        T new_value = glm::mix(
            state->origin_value, state->destination_value, t
        );
        (node->*F_setter)(new_value);
    }
};


// TODO use template magic to merge this template class with above one
// check: if constexpr (C++17)
template<typename T,
         typename N,
         N Node::*N_member,
         T (N::*F_getter)(),
         void(N::*F_setter)(const T&)>
class SpecializedNodeAttributeTransition : public NodeTransitionBase {
    AttributeTransitionMethod _advance_method;
    T _value_advance;

    public:
    SpecializedNodeAttributeTransition(T value_advance, const AttributeTransitionMethod& advance_method,
                            const double duration, const TransitionWarping& warping = TransitionWarping())
    : NodeTransitionBase(duration, warping), _value_advance(value_advance), _advance_method(advance_method)
    {
    }

    SpecializedNodeAttributeTransition(T value_advance, const double duration,
                            const TransitionWarping& warping = TransitionWarping())
    : SpecializedNodeAttributeTransition(value_advance, AttributeTransitionMethod::set, duration, warping)
    {
    }

    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const
    {
        const N* spec_node = &node->*N_member;
        return std::make_unique<NodeAttributeTransitionState<T>>(
            (spec_node->*F_getter)(), this->_value_advance, this->_advance_method
        );
    }

    void evaluate(TransitionStateBase* state_b, Node* node, const double t) const
    {
        auto state = static_cast<NodeAttributeTransitionState<T>*>(state_b);
        N* spec_node = &node->*N_member;
        T new_value = glm::mix(
            state->origin_value, state->destination_value, t
        );
        (spec_node->*F_setter)(new_value);
    }
};


typedef NodeAttributeTransition<glm::dvec2,
                                &Node::position, &Node::position> \
        NodePositionTransition;

typedef NodeAttributeTransition<double,
                                &Node::rotation, &Node::rotation> \
        NodeRotationTransition;

typedef NodeAttributeTransition<glm::dvec2,
                                &Node::scale, &Node::scale> \
        NodeScaleTransition;

typedef NodeAttributeTransition<glm::dvec4,
                                &Node::color, &Node::color> \
        NodeColorTransition;


typedef SpecializedNodeAttributeTransition<glm::dvec2,
                                           BodyNode, &Node::body,
                                           &BodyNode::velocity, &BodyNode::velocity> \
        BodyNodeVelocityTransition;

typedef SpecializedNodeAttributeTransition<double,
                                           BodyNode, &Node::body,
                                           &BodyNode::angular_velocity, &BodyNode::angular_velocity> \
        BodyNodeAngularVelocityTransition;

} // namespace kaacore
