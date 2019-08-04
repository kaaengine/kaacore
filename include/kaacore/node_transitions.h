#pragma once

#include <memory>

#include <glm/glm.hpp>

#include "kaacore/transitions.h"
#include "kaacore/nodes.h"


namespace kaacore {

template<typename T,
         T (Node::*F_getter)(),
         void(Node::*F_setter)(const T&)>
class NodeAttributeTransition : public NodeTransitionBase {
    T _value_advance;

    struct _State : TransitionStateBase {
        T origin_value;

        _State(T value) : origin_value(value)
        {
        }
    };

    public:
    NodeAttributeTransition(T value_advance, const double duration)
    : NodeTransitionBase(duration), _value_advance(value_advance)
    {
    }

    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const
    {
        return std::make_unique<_State>((node->*F_getter)());
    }

    void evaluate(TransitionStateBase* state_b, Node* node, const double t) const
    {
        auto state = static_cast<_State*>(state_b);
        T new_value = glm::mix(
            state->origin_value,
            state->origin_value + this->_value_advance,
            t
        );
        (node->*F_setter)(new_value);
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

} // namespace kaacore
