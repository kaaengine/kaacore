#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <vector>

#include <glm/glm.hpp>

namespace kaacore {

struct Node;

class NodeTransitionBase;
typedef std::shared_ptr<const NodeTransitionBase> NodeTransitionHandle;

struct TransitionStateBase {};

struct TransitionTimePoint {
    double abs_t;
    bool is_backing;
    uint32_t cycle_index;
};

struct TransitionWarping {
    uint32_t loops;
    bool back_and_forth;
    uint64_t easing;

    TransitionWarping(uint32_t loops = 1, bool back_and_forth = false);

    double duration_factor() const;
    TransitionTimePoint warp_time(
        const TransitionTimePoint& tp, const double internal_duration) const;
};

class NodeTransitionBase {
  public:
    double duration;
    double internal_duration;

    TransitionWarping warping;

    NodeTransitionBase();
    NodeTransitionBase(
        const double duration,
        const TransitionWarping& warping = TransitionWarping());

    virtual std::unique_ptr<TransitionStateBase> prepare_state(
        Node* node) const;
    virtual void process_time_point(
        TransitionStateBase* state, Node* node,
        const TransitionTimePoint& tp) const = 0;
};

class NodeTransitionCustomizable : public NodeTransitionBase {
  public:
    using NodeTransitionBase::NodeTransitionBase;

    virtual void process_time_point(
        TransitionStateBase* state, Node* node,
        const TransitionTimePoint& tp) const;
    virtual void evaluate(
        TransitionStateBase* state, Node* node, const double t) const = 0;
};

class NodeTransitionsGroupBase : public NodeTransitionBase {
    struct _SubTransition {
        NodeTransitionHandle handle;
        double starting_time;
        double ending_time;

        _SubTransition(
            const NodeTransitionHandle& handle, const double starting_time,
            const double ending_time)
            : handle(handle), starting_time(starting_time),
              ending_time(ending_time)
        {}
    };

  protected:
    std::vector<_SubTransition> _sub_transitions;
    bool has_infinite_sub_transitions;
};

class NodeTransitionsSequence : public NodeTransitionsGroupBase {
  public:
    NodeTransitionsSequence(
        const std::vector<NodeTransitionHandle>& transitions,
        const TransitionWarping& warping = TransitionWarping()) noexcept(false);
    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
    void process_time_point(
        TransitionStateBase* state, Node* node,
        const TransitionTimePoint& tp) const;
};

class NodeTransitionsParallel : public NodeTransitionsGroupBase {
  public:
    NodeTransitionsParallel(
        const std::vector<NodeTransitionHandle>& transitions,
        const TransitionWarping& warping = TransitionWarping()) noexcept(false);
    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
    void process_time_point(
        TransitionStateBase* state, Node* node,
        const TransitionTimePoint& tp) const;
};

class NodeTransitionDelay : public NodeTransitionBase {
  public:
    NodeTransitionDelay(const double duration);
    void process_time_point(
        TransitionStateBase* state, Node* node,
        const TransitionTimePoint& tp) const;
};

typedef std::function<void(Node*)> NodeTransitionCallbackFunc;

class NodeTransitionCallback : public NodeTransitionBase {
    NodeTransitionCallbackFunc callback_func;

  public:
    NodeTransitionCallback(const NodeTransitionCallbackFunc& func);
    void process_time_point(
        TransitionStateBase* state, Node* node,
        const TransitionTimePoint& tp) const;
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

template<class T, class... Args>
NodeTransitionHandle
make_node_transition(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
};

template<class... Args>
NodeTransitionHandle
make_node_transitions_sequence(
    const std::vector<NodeTransitionHandle>& transitions, Args&&... args)
{
    return std::make_shared<NodeTransitionsSequence>(
        transitions, std::forward<Args>(args)...);
}

template<class... Args>
NodeTransitionHandle
make_node_transitions_parallel(
    const std::vector<NodeTransitionHandle>& transitions, Args&&... args)
{
    return std::make_shared<NodeTransitionsParallel>(
        transitions, std::forward<Args>(args)...);
}

} // namespace kaacore
