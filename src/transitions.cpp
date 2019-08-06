#include <cmath>

#include "kaacore/nodes.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/transitions.h"


namespace kaacore {


NodeTransitionBase::NodeTransitionBase()
: duration(std::nan(""))
{
}

NodeTransitionBase::NodeTransitionBase(const double dur)
: duration(dur)
{
}

std::unique_ptr<TransitionStateBase> NodeTransitionBase::prepare_state(Node* node) const
{
    return nullptr;
}


struct _NodeTransitionsGroupSubState {
    NodeTransitionHandle handle;
    std::unique_ptr<TransitionStateBase> state;
    bool state_prepared;
    bool finished;
    double ending_t;

    _NodeTransitionsGroupSubState(const NodeTransitionHandle& transition_handle, const double ending_t)
    : handle(transition_handle), ending_t(ending_t),
      state(nullptr), state_prepared(false), finished(false)
    {
    }
};


struct _NodeTransitionsGroupState : TransitionStateBase {
    std::vector<_NodeTransitionsGroupSubState> sub_states;
};


std::unique_ptr<TransitionStateBase> NodeTransitionsGroupBase::prepare_state(Node* node) const
{
    auto sequence_state = std::make_unique<_NodeTransitionsGroupState>();
    for (const auto& sub_tr : this->_sub_transitions) {
        sequence_state->sub_states.emplace_back(
            sub_tr.handle, sub_tr.ending_time / this->duration
        );
    }

    return sequence_state;
}


NodeTransitionsSequence::NodeTransitionsSequence(
    const std::vector<NodeTransitionHandle>& transitions) noexcept(false)
{
    double total_duration = 0.;
    for (const auto& tr : transitions) {
        KAACORE_CHECK(tr->duration >= 0.);
        total_duration += tr->duration;
        this->_sub_transitions.emplace_back(tr, total_duration);
    }
    this->duration = total_duration;
}

void NodeTransitionsSequence::evaluate(TransitionStateBase* state_b, Node* node, const double t) const
{
    double current_starting_t = 0.;
    auto state = static_cast<_NodeTransitionsGroupState*>(state_b);
    for (auto& sub_state : state->sub_states) {
        if (not sub_state.state_prepared) {
            sub_state.state = sub_state.handle->prepare_state(node);
            sub_state.state_prepared = true;
        }
        if (not sub_state.finished) {
            double sub_t = (t - current_starting_t) / (sub_state.ending_t - current_starting_t);
            sub_state.handle->evaluate(
                sub_state.state.get(), node, glm::min(sub_t, 1.)
            );
            if (sub_t >= 1.) {
                sub_state.finished = true;
            } else {
                break;
            }
        }
        current_starting_t = sub_state.ending_t;
    }
}


NodeTransitionsParallel::NodeTransitionsParallel(
    const std::vector<NodeTransitionHandle>& transitions) noexcept(false)
{
    double max_duration = 0.;
    for (const auto& tr : transitions) {
        KAACORE_CHECK(tr->duration >= 0.);
        max_duration = glm::max(max_duration, tr->duration);
        this->_sub_transitions.emplace_back(tr, tr->duration);
    }
    this->duration = max_duration;
}

void NodeTransitionsParallel::evaluate(TransitionStateBase* state_b, Node* node, const double t) const
{
    auto state = static_cast<_NodeTransitionsGroupState*>(state_b);
    for (auto& sub_state : state->sub_states) {
        if (not sub_state.state_prepared) {
            sub_state.state = sub_state.handle->prepare_state(node);
            sub_state.state_prepared = true;
        }
        if (not sub_state.finished) {
            double sub_t = t / sub_state.ending_t;
            sub_state.handle->evaluate(
                sub_state.state.get(), node, glm::min(sub_t, 1.)
            );
            if (sub_t >= 1.) {
                sub_state.finished = true;
            }
        }
    }
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
        this->transition_state.get(), node, glm::min(t, 1.)
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
