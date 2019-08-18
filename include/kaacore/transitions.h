#pragma once

#include <cmath>
#include <memory>
#include <vector>

#include <glm/glm.hpp>


namespace kaacore {

struct Node;

class NodeTransitionBase;
typedef std::shared_ptr<const NodeTransitionBase> NodeTransitionHandle;


struct TransitionStateBase {
};


struct TransitionTimePoint {
    double abs_t;
    bool is_backing;
    uint32_t cycle_index;

    bool operator<(const TransitionTimePoint& other) const;
    bool operator==(const TransitionTimePoint& other) const;
};


struct TransitionWarping {
    uint32_t loops;
    bool back_and_forth;
    uint64_t easing;

    TransitionWarping(uint32_t loops = 1, bool back_and_forth = false)
    : loops(loops), back_and_forth(back_and_forth)
    {
        // KAACORE_ASSERT(this->loops >= 0);
    }

    double duration_factor() const
    {
        if (this->loops == 0) {
            return INFINITY;
        } else {
            return double(this->loops) * (1 + int(this->back_and_forth));
        }
    }

    TransitionTimePoint warp_time(const TransitionTimePoint& tp, const double internal_duration) const
    {
        double duration_factor = (1 + int(this->back_and_forth));
        double warped_abs_t = glm::mod(tp.abs_t, internal_duration * duration_factor);

        // prevent floating errors from resetting cycle
        uint32_t cycle_index = tp.abs_t / (internal_duration * duration_factor);
        if (this->loops > 0 and cycle_index >= this->loops) {
            warped_abs_t = internal_duration * duration_factor;
            cycle_index--;
        }

        if (this->back_and_forth and warped_abs_t > internal_duration) {
            warped_abs_t = fabs(2 * internal_duration - warped_abs_t);
            return TransitionTimePoint{warped_abs_t, (tp.is_backing != true), cycle_index};
        } else {
            return TransitionTimePoint{warped_abs_t, (tp.is_backing != false), cycle_index};
        }
    }
};


class NodeTransitionBase {
    public:
    double duration;
    double internal_duration;

    TransitionWarping warping;

    NodeTransitionBase();
    NodeTransitionBase(const double duration, const TransitionWarping& warping = TransitionWarping());

    virtual std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
    virtual void evaluate_abs(TransitionStateBase* state, Node* node, const TransitionTimePoint& tp) const;
    virtual void evaluate(TransitionStateBase* state, Node* node, const double t) const = 0;
};


class NodeTransitionsGroupBase : public NodeTransitionBase {
    struct _SubTransition {
        NodeTransitionHandle handle;
        double starting_time;
        double ending_time;

        _SubTransition(const NodeTransitionHandle& handle, const double starting_time, const double ending_time)
        : handle(handle), starting_time(starting_time), ending_time(ending_time)
        {
        }
    };

    protected:
    std::vector<_SubTransition> _sub_transitions;

    public:
    // std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
    void evaluate(TransitionStateBase* state, Node* node, const double t) const;
};


class NodeTransitionsSequence : public NodeTransitionsGroupBase {
    public:
    NodeTransitionsSequence(const std::vector<NodeTransitionHandle>& transitions, const TransitionWarping& warping=TransitionWarping()) noexcept(false);
    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
    void evaluate_abs(TransitionStateBase* state, Node* node, const TransitionTimePoint& tp) const;
};


class NodeTransitionsParallel : public NodeTransitionsGroupBase {
    public:
    NodeTransitionsParallel(const std::vector<NodeTransitionHandle>& transitions, const TransitionWarping& warping=TransitionWarping()) noexcept(false);
    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
    void evaluate_abs(TransitionStateBase* state, Node* node, const TransitionTimePoint& tp) const;
};


class NodeTransitionDelay : public NodeTransitionBase {
    public:
    NodeTransitionDelay(const double duration);
    void evaluate(TransitionStateBase* state, Node* node, const double t) const;
};


typedef std::function<void(Node*)> NodeTransitionCallbackFunc;

class NodeTransitionCallback : public NodeTransitionBase {
    NodeTransitionCallbackFunc callback_func;

    public:
    NodeTransitionCallback(const NodeTransitionCallbackFunc& func);
    void evaluate_abs(TransitionStateBase* state, Node* node, const TransitionTimePoint& tp) const;
    void evaluate(TransitionStateBase* state, Node* node, const double t) const;
};


struct NodeTransitionRunner {
    NodeTransitionHandle transition_handle;
    std::unique_ptr<TransitionStateBase> transition_state;
    bool transition_state_prepared = false;
    bool finished = false;
    uint64_t current_time = 0;

    void setup(const NodeTransitionHandle& transition);
    void step(Node* node, const uint32_t dt);
    operator bool() const;
};


template <class T, class... Args>
NodeTransitionHandle make_node_transition(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
};

template <class... Args>
NodeTransitionHandle make_node_transitions_sequence(const std::vector<NodeTransitionHandle>& transitions, Args&&... args) 
{
    return std::make_shared<NodeTransitionsSequence>(transitions, std::forward<Args>(args)...);
}

template <class... Args>
NodeTransitionHandle make_node_transitions_parallel(const std::vector<NodeTransitionHandle>& transitions, Args&&... args) 
{
    return std::make_shared<NodeTransitionsParallel>(transitions, std::forward<Args>(args)...);
}

} // namespace kaacore
