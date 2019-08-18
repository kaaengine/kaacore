#include <list>

#include "kaacore/nodes.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/transitions.h"


namespace kaacore {

NodeTransitionBase::NodeTransitionBase()
: duration(std::nan("")), internal_duration(std::nan(""))
{
}

NodeTransitionBase::NodeTransitionBase(const double dur, const TransitionWarping& warping)
: duration(dur * warping.duration_factor()), internal_duration(dur), warping(warping)
{
}

std::unique_ptr<TransitionStateBase> NodeTransitionBase::prepare_state(Node* node) const
{
    return nullptr;
}

void NodeTransitionBase::evaluate_abs(TransitionStateBase* state, Node* node, const TransitionTimePoint& tp) const
{
    KAACORE_ASSERT(this->duration > 0.);
    const TransitionTimePoint local_tp = this->warping.warp_time(tp, this->internal_duration);
    const double warped_t = local_tp.abs_t / this->internal_duration;
    // log(" * abs_t: %lf, warped_t: %lf, duration: %lf", tp.abs_t, warped_t, this->internal_duration);
    this->evaluate(state, node, warped_t);
}


struct _NodeTransitionsGroupSubState {
    NodeTransitionHandle handle;
    std::unique_ptr<TransitionStateBase> state;
    bool state_prepared;
    double starting_abs_t;
    double ending_abs_t;

    _NodeTransitionsGroupSubState(const NodeTransitionHandle& transition_handle, const double starting_abs_t, const double ending_abs_t)
    : handle(transition_handle), starting_abs_t(starting_abs_t), ending_abs_t(ending_abs_t),
      state(nullptr), state_prepared(false)
    {
    }
};


void NodeTransitionsGroupBase::evaluate(TransitionStateBase* state, Node* node, const double t) const
{
    KAACORE_CHECK(!"UNREACHABLE");
}


struct _NodeTransitionsSequenceState : TransitionStateBase {
    TransitionTimePoint prev_tp;
    std::list<_NodeTransitionsGroupSubState> sub_states;
    std::list<_NodeTransitionsGroupSubState>::iterator sub_state_it;
};


NodeTransitionsSequence::NodeTransitionsSequence(
    const std::vector<NodeTransitionHandle>& transitions, const TransitionWarping& warping) noexcept(false)
{
    double total_duration = 0.;
    for (const auto& tr : transitions) {
        KAACORE_CHECK(tr->duration >= 0.);
        this->_sub_transitions.emplace_back(tr, total_duration, total_duration + tr->duration);
        total_duration += tr->duration;
    }
    this->warping = warping;
    this->duration = total_duration * warping.duration_factor();
    this->internal_duration = total_duration;
}

std::unique_ptr<TransitionStateBase> NodeTransitionsSequence::prepare_state(Node* node) const
{
    auto sequence_state = std::make_unique<_NodeTransitionsSequenceState>();
    for (const auto& sub_tr : this->_sub_transitions) {
        sequence_state->sub_states.emplace_back(
            sub_tr.handle, sub_tr.starting_time, sub_tr.ending_time
        );
    }
    sequence_state->sub_state_it = sequence_state->sub_states.begin();

    return sequence_state;
}

void NodeTransitionsSequence::evaluate_abs(TransitionStateBase* state_b, Node* node, const TransitionTimePoint& tp) const
{
    const TransitionTimePoint warped_tp = this->warping.warp_time(tp, this->internal_duration);
    auto state = static_cast<_NodeTransitionsSequenceState*>(state_b);
    auto it = state->sub_state_it;
    uint32_t cur_cycle_index = state->prev_tp.cycle_index;
    bool is_backing = state->prev_tp.is_backing;
    // log("abs_t: %lg, warped abs_t: %lg, backing: %u, target_backing: %u, parent_backing: %u",
        // tp.abs_t, warped_tp.abs_t, is_backing, warped_tp.is_backing, tp.is_backing);

    while (cur_cycle_index <= warped_tp.cycle_index) {
        // log("cur_cycle_index: %u, target_cycle_index: %u", cur_cycle_index, warped_tp.cycle_index);
        if (not it->state_prepared) {
            it->state = it->handle->prepare_state(node);
            it->state_prepared = true;
        }

        const double sub_abs_t = glm::clamp(
            warped_tp.abs_t - it->starting_abs_t,
            0., it->handle->duration
        );
        const TransitionTimePoint sub_tp = TransitionTimePoint{sub_abs_t, is_backing};

        // log("sub_abs_t: %lf, starting: %lf, ending: %lf", sub_abs_t, it->starting_abs_t, it->ending_abs_t);

        it->handle->evaluate_abs(it->state.get(), node, sub_tp);

        if (cur_cycle_index == warped_tp.cycle_index and
            is_backing == warped_tp.is_backing and
            it->starting_abs_t <= warped_tp.abs_t and 
            it->ending_abs_t >= warped_tp.abs_t) {
            break;
        }
        // log("! going to next transition in sequence");

        if (not is_backing) {
            it++;
            if (it == state->sub_states.end()) {
                if (this->warping.back_and_forth or tp.is_backing) {
                    is_backing = true;
                    it--;
                } else {
                    it = state->sub_states.begin();
                    cur_cycle_index++;
                }
            }
        } else {
            if (it == state->sub_states.begin()) {
                is_backing = false;
                cur_cycle_index++;
            } else {
                it--;
            }
        }
    }

    state->prev_tp = warped_tp;
    state->sub_state_it = it;
}


struct _NodeTransitionsParallelSubState : _NodeTransitionsGroupSubState {
    bool sleeping;

    using _NodeTransitionsGroupSubState::_NodeTransitionsGroupSubState;
};


struct _NodeTransitionsParallelState : TransitionStateBase {
    TransitionTimePoint prev_tp;
    std::vector<_NodeTransitionsParallelSubState> sub_states;
};


NodeTransitionsParallel::NodeTransitionsParallel(
    const std::vector<NodeTransitionHandle>& transitions, const TransitionWarping& warping) noexcept(false)
{
    double max_duration = 0.;
    for (const auto& tr : transitions) {
        KAACORE_CHECK(tr->duration >= 0.);
        max_duration = glm::max(max_duration, tr->duration);
        this->_sub_transitions.emplace_back(tr, 0., tr->duration);
    }
    this->warping = warping;
    this->duration = max_duration * warping.duration_factor();
    this->internal_duration = max_duration;
}

std::unique_ptr<TransitionStateBase> NodeTransitionsParallel::prepare_state(Node* node) const
{
    auto sequence_state = std::make_unique<_NodeTransitionsParallelState>();
    for (const auto& sub_tr : this->_sub_transitions) {
        sequence_state->sub_states.emplace_back(
            sub_tr.handle, sub_tr.starting_time, sub_tr.ending_time
        );
    }

    return sequence_state;
}

void NodeTransitionsParallel::evaluate_abs(TransitionStateBase* state_b, Node* node, const TransitionTimePoint& tp) const
{
    auto state = static_cast<_NodeTransitionsParallelState*>(state_b);
    const TransitionTimePoint warped_tp = this->warping.warp_time(tp, this->internal_duration);
    uint32_t cur_cycle_index = state->prev_tp.cycle_index;
    bool is_backing = state->prev_tp.is_backing;
    // log("abs_t: %lf, warped abs_t: %lf", abs_t, warped_tp.abs_t);
    // log("parallel sub_states count: %u", state->sub_states.size());
    // log("cur_cycle_index: %u, target_cycle_index: %u", cur_cycle_index, warped_tp.cycle_index);

    while (cur_cycle_index <= warped_tp.cycle_index) {
        // log("cur_cycle_index: %u, target_cycle_index: %u", cur_cycle_index, warped_tp.cycle_index);
        for (auto& sub_state : state->sub_states) {
            if (not sub_state.state_prepared) {
                sub_state.state = sub_state.handle->prepare_state(node);
                sub_state.state_prepared = true;
            }

            double sub_abs_t;
            bool sub_fits;
            if (cur_cycle_index < state->prev_tp.cycle_index) {
                if (is_backing) {
                    sub_abs_t = 0.;
                } else {
                    sub_abs_t = sub_state.handle->duration;
                }
                sub_fits = true;
            } else {
                sub_abs_t = glm::clamp(
                    warped_tp.abs_t - sub_state.starting_abs_t,
                    0., sub_state.handle->duration
                );
                sub_fits = (warped_tp.abs_t > sub_state.starting_abs_t and
                            warped_tp.abs_t < sub_state.ending_abs_t);
            }
            // log("[%p] sub_abs_t: %lf, starting: %lf, ending: %lf",
                // &sub_state, sub_abs_t, sub_state.starting_abs_t, sub_state.ending_abs_t);
            const TransitionTimePoint sub_tp = TransitionTimePoint{sub_abs_t, is_backing};

            if (not sub_state.sleeping) {
                sub_state.handle->evaluate_abs(sub_state.state.get(), node, sub_tp);
                if (not sub_fits) {
                    sub_state.sleeping = true;
                }
            } else {
                if (sub_fits) {
                    // TODO consider calling sub-transition edges
                    sub_state.handle->evaluate_abs(sub_state.state.get(), node, sub_tp);
                    sub_state.sleeping = false;
                }
            }
        }

        if (this->warping.back_and_forth) {
            if (is_backing) {
                cur_cycle_index++;
            }
            is_backing = !is_backing;

            if (!warped_tp.is_backing and is_backing) {
                break;
            }
        } else {
            cur_cycle_index++;
        }
    }

    state->prev_tp = warped_tp;
}


NodeTransitionDelay::NodeTransitionDelay(const double duration)
: NodeTransitionBase(duration)
{
}

void NodeTransitionDelay::evaluate(TransitionStateBase* state, Node* node, const double t) const
{
}


NodeTransitionCallback::NodeTransitionCallback(const NodeTransitionCallbackFunc& func)
: callback_func(func), NodeTransitionBase(0.)
{
}

void NodeTransitionCallback::evaluate_abs(TransitionStateBase* state, Node* node, const TransitionTimePoint& tp) const
{
    KAACORE_ASSERT(this->callback_func);
    this->callback_func(node);
}

void NodeTransitionCallback::evaluate(TransitionStateBase* state, Node* node, const double t) const
{
    KAACORE_CHECK(!"UNREACHABLE");
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
    this->transition_handle->evaluate_abs(
        this->transition_state.get(), node, TransitionTimePoint{double(this->current_time)}
    );
    if (this->current_time >= this->transition_handle->duration) {
        this->finished = true;
    }
}

NodeTransitionRunner::operator bool() const
{
    return bool(this->transition_handle);
}


} // namespace kaacore
