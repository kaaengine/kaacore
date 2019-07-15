#include "kaacore/nodes.h"
#include "kaacore/exceptions.h"

#include "kaacore/transitions.h"


namespace kaacore {

NodeTransitionBase::NodeTransitionBase(const double dur)
: duration(dur)
{
}


struct NodePositionTransitionState : TransitionStateBase {
    glm::dvec2 node_starting_position;

    NodePositionTransitionState(glm::dvec2 pos)
    : node_starting_position(pos)
    {
    }
};


NodePositionTransition::NodePositionTransition(const glm::dvec2& move_vector, const double dur)
: NodeTransitionBase(dur), _move_vector(move_vector)
{
}

std::unique_ptr<TransitionStateBase>
NodePositionTransition::prepare_state(Node* node) const
{
    return std::make_unique<NodePositionTransitionState>(node->position);
}

void NodePositionTransition::evaluate(TransitionStateBase* state_b,
                                      Node* node, double t) const
{
    auto state = static_cast<NodePositionTransitionState*>(state_b);
    node->set_position(
        glm::mix(state->node_starting_position,
                 state->node_starting_position + this->_move_vector, t)
    );
}


void NodeTransitionRunner::setup(const NodeTransitionHandle& transition)
{
    this->transition_handle = transition;
    this->transition_state.reset();
    this->transition_state_prepared = false;
    this->finished = false;
    this->current_time = 0;
}

void NodeTransitionRunner::step(Node* node, const uint32_t dt)
{
    KAACORE_ASSERT(bool(*this));
    if (this->finished) {
        return;
    }

    if (not this->transition_state_prepared) {
        this->transition_state = \
            this->transition_handle->prepare_state(node);
        this->transition_state_prepared = true;
    }

    this->current_time += dt;
    double t = double(this->current_time) / this->transition_handle->duration;
    this->transition_handle->evaluate(
        this->transition_state.get(), node, t
    );
    if (t >= 1.) {
        this->finished = true;
    }
}

NodeTransitionRunner::operator bool() const
{
    return bool(this->transition_handle);
}


} // namespace kaacore
