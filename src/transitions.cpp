#include <list>

#include <spdlog/fmt/fmt.h>

#include "kaacore/easings.h"
#include "kaacore/exceptions.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"

#include "kaacore/transitions.h"

namespace kaacore {

TransitionWarping::TransitionWarping(uint32_t loops, bool back_and_forth)
    : loops(loops), back_and_forth(back_and_forth)
{
    KAACORE_ASSERT(
        this->loops >= 0, "Number of loop must be greater or equal to zero.");
}

double
TransitionWarping::duration_factor() const
{
    if (this->loops == 0) {
        return INFINITY;
    } else {
        return double(this->loops) * (1 + int(this->back_and_forth));
    }
}

TransitionTimePoint
TransitionWarping::warp_time(
    const TransitionTimePoint& tp, const Seconds internal_duration) const
{
    double duration_factor = (1 + int(this->back_and_forth));
    Seconds warped_abs_t = Seconds(glm::mod(
        tp.abs_t.count(), (internal_duration * duration_factor).count()));

    // prevent floating errors from resetting cycle
    uint32_t cycle_index = tp.abs_t / (internal_duration * duration_factor);
    if (this->loops > 0 and cycle_index >= this->loops) {
        warped_abs_t = internal_duration * duration_factor;
        cycle_index--;
    }

    if (this->back_and_forth and warped_abs_t > internal_duration) {
        warped_abs_t = std::chrono::abs(2 * internal_duration - warped_abs_t);
        return TransitionTimePoint{warped_abs_t, (tp.is_backing != true),
                                   cycle_index};
    } else {
        return TransitionTimePoint{warped_abs_t, (tp.is_backing != false),
                                   cycle_index};
    }
}

NodeTransitionBase::NodeTransitionBase()
    : duration(std::nan("")), internal_duration(std::nan(""))
{}

NodeTransitionBase::NodeTransitionBase(
    const Seconds dur, const TransitionWarping& warping)
    : duration(dur * warping.duration_factor()), internal_duration(dur),
      warping(warping)
{}

std::unique_ptr<TransitionStateBase>
NodeTransitionBase::prepare_state(NodePtr node) const
{
    return nullptr;
}

NodeTransitionCustomizable::NodeTransitionCustomizable() {}

NodeTransitionCustomizable::NodeTransitionCustomizable(
    const Seconds duration, const TransitionWarping& warping,
    const Easing easing)
    : NodeTransitionBase(duration, warping), _easing(easing)
{}

void
NodeTransitionCustomizable::process_time_point(
    TransitionStateBase* state, NodePtr node,
    const TransitionTimePoint& tp) const
{
    KAACORE_ASSERT(
        this->duration >= 0.s, "Duration must be greater than zero.");
    const TransitionTimePoint local_tp =
        this->warping.warp_time(tp, this->internal_duration);

    KAACORE_LOG_TRACE(
        "NodeTransitionCustomizable({})::process_time_point - node: {}, abs_t: "
        "{}, local_abs_t: {}, internal_duration: {}",
        fmt::ptr(this), fmt::ptr(node.get()), tp.abs_t.count(),
        local_tp.abs_t.count(), this->internal_duration.count());

    if (this->duration > 0.s) {
        const double warped_t = local_tp.abs_t / this->internal_duration;
        this->evaluate(state, node, ease(this->_easing, warped_t));
    } else {
        this->evaluate(state, node, tp.is_backing ? 0. : 1.);
    }
}

struct _NodeTransitionsGroupSubState {
    NodeTransitionHandle handle;
    std::unique_ptr<TransitionStateBase> state;
    bool state_prepared;
    Seconds starting_abs_t;
    Seconds ending_abs_t;

    _NodeTransitionsGroupSubState(
        const NodeTransitionHandle& transition_handle,
        const Seconds starting_abs_t, const Seconds ending_abs_t)
        : handle(transition_handle), starting_abs_t(starting_abs_t),
          ending_abs_t(ending_abs_t), state(nullptr), state_prepared(false)
    {}
};

NodeTransitionsGroupBase::NodeTransitionsGroupBase(
    const std::vector<NodeTransitionHandle>& transitions) noexcept(false)
{
    KAACORE_ASSERT(transitions.size(), "At least one transition is required.");
}

struct _NodeTransitionsSequenceState : TransitionStateBase {
    TransitionTimePoint prev_tp;
    std::list<_NodeTransitionsGroupSubState> sub_states;
    std::list<_NodeTransitionsGroupSubState>::iterator sub_state_it;
};

NodeTransitionsSequence::NodeTransitionsSequence(
    const std::vector<NodeTransitionHandle>& transitions,
    const TransitionWarping& warping) noexcept(false)
    : NodeTransitionsGroupBase(transitions)
{
    Seconds total_duration = 0.s;
    bool has_infinite_subs = false;

    for (const auto& tr : transitions) {
        KAACORE_CHECK(
            tr->duration >= 0.s, "Duration must be greater than zero.");

        Seconds sub_duration;
        if (has_infinite_subs) {
            throw exception("NodeTransitionsSequence has infinite "
                            "subtransition on non last position");
        }

        if (std::isinf(tr->duration.count())) {
            sub_duration = tr->internal_duration;
            has_infinite_subs = true;
        } else {
            sub_duration = tr->duration;
        }
        this->_sub_transitions.emplace_back(
            tr, total_duration, total_duration + tr->duration);
        total_duration += sub_duration;
    }
    this->has_infinite_sub_transitions = has_infinite_subs;
    this->warping = warping;
    if (has_infinite_subs) {
        // TODO force user to set those implicitly?
        this->warping.loops = 0;
        this->warping.back_and_forth = false;
    }
    this->duration = total_duration * this->warping.duration_factor();
    this->internal_duration = total_duration;

    KAACORE_LOG_DEBUG(
        "NodeTransitionsSequence({}) constructed - duration: {}, "
        "internal_duration: {}",
        fmt::ptr(this), this->duration.count(),
        this->internal_duration.count());
}

std::unique_ptr<TransitionStateBase>
NodeTransitionsSequence::prepare_state(NodePtr node) const
{
    auto sequence_state = std::make_unique<_NodeTransitionsSequenceState>();
    for (const auto& sub_tr : this->_sub_transitions) {
        sequence_state->sub_states.emplace_back(
            sub_tr.handle, sub_tr.starting_time, sub_tr.ending_time);
    }
    sequence_state->sub_state_it = sequence_state->sub_states.begin();

    return sequence_state;
}

void
NodeTransitionsSequence::process_time_point(
    TransitionStateBase* state_b, NodePtr node,
    const TransitionTimePoint& tp) const
{
    const TransitionTimePoint warped_tp =
        this->has_infinite_sub_transitions
            ? tp
            : this->warping.warp_time(tp, this->internal_duration);
    auto state = static_cast<_NodeTransitionsSequenceState*>(state_b);
    auto it = state->sub_state_it;
    uint32_t cur_cycle_index = state->prev_tp.cycle_index;
    bool is_backing = state->prev_tp.is_backing;

    KAACORE_LOG_TRACE(
        "NodeTransitionsSequence({})::process_time_point - node: {}, abs_t: "
        "{}, warped_abs_t: {}",
        fmt::ptr(this), fmt::ptr(node.get()), tp.abs_t.count(),
        warped_tp.abs_t.count());

    while (cur_cycle_index <= warped_tp.cycle_index) {
        if (not it->state_prepared) {
            KAACORE_LOG_TRACE(
                "NodeTransitionsSequence({})::process_time_point - preparing "
                "state",
                fmt::ptr(this));
            it->state = it->handle->prepare_state(node);
            it->state_prepared = true;
        }

        const Seconds sub_abs_t = std::clamp<Seconds>(
            warped_tp.abs_t - it->starting_abs_t, 0.s, it->handle->duration);
        const TransitionTimePoint sub_tp =
            TransitionTimePoint{sub_abs_t, is_backing};

        KAACORE_LOG_TRACE(
            "NodeTransitionsSequence({})::process_time_point - processing "
            "sub-transition, "
            "sub_abs_t: {}",
            fmt::ptr(this), sub_abs_t.count());
        it->handle->process_time_point(it->state.get(), node, sub_tp);

        if (node.is_marked_to_delete()) {
            return;
        }

        if (cur_cycle_index == warped_tp.cycle_index and
            is_backing == warped_tp.is_backing and
            it->starting_abs_t <= warped_tp.abs_t and
            it->ending_abs_t > warped_tp.abs_t) {
            KAACORE_LOG_TRACE(
                "NodeTransitionsSequence({})::process_time_point - met "
                "breaking condition",
                fmt::ptr(this));
            break;
        }

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

    _NodeTransitionsParallelSubState(
        const NodeTransitionHandle& transition_handle,
        const Seconds starting_abs_t, const Seconds ending_abs_t)
        : _NodeTransitionsGroupSubState(
              transition_handle, starting_abs_t, ending_abs_t),
          sleeping(false)
    {}
};

struct _NodeTransitionsParallelState : TransitionStateBase {
    TransitionTimePoint prev_tp;
    std::vector<_NodeTransitionsParallelSubState> sub_states;
};

NodeTransitionsParallel::NodeTransitionsParallel(
    const std::vector<NodeTransitionHandle>& transitions,
    const TransitionWarping& warping) noexcept(false)
    : NodeTransitionsGroupBase(transitions)
{
    Seconds max_sub_internal_duration = 0.s;
    bool has_infinite_subs = false;

    for (const auto& tr : transitions) {
        Seconds sub_duration;
        KAACORE_CHECK(
            tr->duration >= 0.s, "Duration must be greater than zero.");
        if (std::isinf(tr->duration.count())) {
            sub_duration = tr->internal_duration;
            has_infinite_subs = true;
        } else {
            sub_duration = tr->duration;
        }
        max_sub_internal_duration =
            std::max(max_sub_internal_duration, sub_duration);
        this->_sub_transitions.emplace_back(tr, 0.s, tr->duration);
    }
    this->has_infinite_sub_transitions = has_infinite_subs;
    this->warping = warping;
    if (has_infinite_subs) {
        // TODO force user to set those implicitly?
        this->warping.loops = 0;
        this->warping.back_and_forth = false;
    }
    this->duration =
        max_sub_internal_duration * this->warping.duration_factor();
    this->internal_duration = max_sub_internal_duration;

    KAACORE_LOG_DEBUG(
        "NodeTransitionsParallel({}) constructed - duration: {}, "
        "internal_duration: {}",
        fmt::ptr(this), this->duration.count(),
        this->internal_duration.count());
}

std::unique_ptr<TransitionStateBase>
NodeTransitionsParallel::prepare_state(NodePtr node) const
{
    auto sequence_state = std::make_unique<_NodeTransitionsParallelState>();
    for (const auto& sub_tr : this->_sub_transitions) {
        sequence_state->sub_states.emplace_back(
            sub_tr.handle, sub_tr.starting_time, sub_tr.ending_time);
    }

    return sequence_state;
}

void
NodeTransitionsParallel::process_time_point(
    TransitionStateBase* state_b, NodePtr node,
    const TransitionTimePoint& tp) const
{
    auto state = static_cast<_NodeTransitionsParallelState*>(state_b);
    const TransitionTimePoint warped_tp =
        this->has_infinite_sub_transitions
            ? tp
            : this->warping.warp_time(tp, this->internal_duration);
    uint32_t cur_cycle_index = state->prev_tp.cycle_index;
    bool is_backing = state->prev_tp.is_backing;

    KAACORE_LOG_TRACE(
        "NodeTransitionsParallel({})::process_time_point - node: {}, abs_t: "
        "{}, warped_abs_t: {}",
        fmt::ptr(this), fmt::ptr(node.get()), tp.abs_t.count(),
        warped_tp.abs_t.count());

    while (cur_cycle_index <= warped_tp.cycle_index) {
        for (auto& sub_state : state->sub_states) {
            if (not sub_state.state_prepared) {
                sub_state.state = sub_state.handle->prepare_state(node);
                sub_state.state_prepared = true;
            }

            Seconds sub_abs_t;
            bool sub_fits;
            if (cur_cycle_index < state->prev_tp.cycle_index) {
                if (is_backing) {
                    sub_abs_t = 0.s;
                } else {
                    sub_abs_t = sub_state.handle->duration;
                }
                sub_fits = true;
            } else {
                sub_abs_t = std::clamp(
                    warped_tp.abs_t - sub_state.starting_abs_t, 0.s,
                    sub_state.handle->duration);
                sub_fits =
                    (warped_tp.abs_t > sub_state.starting_abs_t and
                     warped_tp.abs_t < sub_state.ending_abs_t);
            }
            const TransitionTimePoint sub_tp =
                TransitionTimePoint{sub_abs_t, is_backing};

            if (not sub_state.sleeping) {
                sub_state.handle->process_time_point(
                    sub_state.state.get(), node, sub_tp);
                if (not sub_fits) {
                    sub_state.sleeping = true;
                }
            } else {
                if (sub_fits) {
                    // TODO consider calling sub-transition edges
                    sub_state.handle->process_time_point(
                        sub_state.state.get(), node, sub_tp);
                    sub_state.sleeping = false;
                }
            }

            if (node.is_marked_to_delete()) {
                return;
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

NodeTransitionDelay::NodeTransitionDelay(const Seconds duration)
    : NodeTransitionBase(duration)
{}

void
NodeTransitionDelay::process_time_point(
    TransitionStateBase* state, NodePtr node,
    const TransitionTimePoint& tp) const
{}

NodeTransitionCallback::NodeTransitionCallback(
    const NodeTransitionCallbackFunc& func)
    : callback_func(func), NodeTransitionBase(0.s)
{}

void
NodeTransitionCallback::process_time_point(
    TransitionStateBase* state, NodePtr node,
    const TransitionTimePoint& tp) const
{
    KAACORE_LOG_TRACE(
        "NodeTransitionCallback({})::process_time_point - node: {}, abs_t: "
        "{}",
        fmt::ptr(this), fmt::ptr(node.get()), tp.abs_t.count());
    KAACORE_ASSERT(this->callback_func, "No callback set.");
    this->callback_func(node);
}

NodeTransitionRunner::NodeTransitionRunner(
    const NodeTransitionHandle& transition)
{
    this->setup(transition);
}

NodeTransitionRunner&
NodeTransitionRunner::operator=(const NodeTransitionHandle& transition)
{
    this->setup(transition);
    return *this;
}

void
NodeTransitionRunner::setup(const NodeTransitionHandle& transition)
{
    this->transition_handle = transition;
    this->transition_state.reset();
    this->transition_state_prepared = false;
    this->current_time = 0us;
}

bool
NodeTransitionRunner::step(NodePtr node, const Microseconds dt)
{
    KAACORE_ASSERT(bool(*this), "Invalid internal stet of transition runner.");

    if (not this->transition_state_prepared) {
        this->transition_state = this->transition_handle->prepare_state(node);
        this->transition_state_prepared = true;
    }

    this->current_time += dt;
    this->transition_handle->process_time_point(
        this->transition_state.get(), node,
        TransitionTimePoint{this->current_time});
    if (this->current_time >= this->transition_handle->duration) {
        return true;
    }
    return false;
}

NodeTransitionRunner::operator bool() const
{
    return bool(this->transition_handle);
}

NodeTransitionHandle
NodeTransitionsManager::get(const std::string& name)
{
    if (not this->_enqueued_updates.empty()) {
        for (auto it = this->_enqueued_updates.rbegin();
             it != this->_enqueued_updates.rend(); it++) {
            const auto& [q_name, q_transition] = *it;
            if (name == q_name) {
                return q_transition;
            }
        }
    }

    const auto it = this->_transitions_map.find(name);
    if (it != this->_transitions_map.end()) {
        return std::get<NodeTransitionRunner>(*it).transition_handle;
    }
    return NodeTransitionHandle();
}

void
NodeTransitionsManager::set(
    const std::string& name, const NodeTransitionHandle& transition)
{
    if (not this->_is_processing) {
        if (transition) {
            this->_transitions_map.insert_or_assign(name, transition);
        } else {
            this->_transitions_map.erase(name);
        }
    } else {
        this->_enqueued_updates.emplace_back(name, transition);
    }
}

void
NodeTransitionsManager::step(NodePtr node, const Microseconds dt)
{
    KAACORE_ASSERT(
        not node.is_marked_to_delete(), "Node is marked for deletion.");
    KAACORE_ASSERT(
        this->_is_processing == false,
        "Invalid internal state of transition manager.");
    this->_is_processing = true;
    for (auto& [name, runner] : this->_transitions_map) {
        bool finished = runner.step(node, dt);
        if (node.is_marked_to_delete()) {
            return;
        }
        if (finished) {
            // if transition is finished destroy it's runner,
            // use `_enqueued_updates` to not break for iteration.
            this->_enqueued_updates.emplace(
                this->_enqueued_updates.begin(), name, NodeTransitionHandle());
        }
    }

    if (not this->_enqueued_updates.empty()) {
        for (const auto& [name, transition] : this->_enqueued_updates) {
            if (transition) {
                this->_transitions_map.insert_or_assign(name, transition);
            } else {
                this->_transitions_map.erase(name);
            }
        }
        this->_enqueued_updates.clear();
    }
    this->_is_processing = false;
}

NodeTransitionsManager::operator bool() const
{
    return not(
        this->_transitions_map.empty() and this->_enqueued_updates.empty());
}

} // namespace kaacore
