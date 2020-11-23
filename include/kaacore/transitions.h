#pragma once

#include <cmath>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "kaacore/clock.h"
#include "kaacore/easings.h"
#include "kaacore/node_ptr.h"

namespace kaacore {

const std::string default_transition_name = "__default__";

class NodeTransitionBase;
typedef std::shared_ptr<const NodeTransitionBase> NodeTransitionHandle;

struct TransitionStateBase {
    virtual ~TransitionStateBase() = default;
};

struct TransitionTimePoint {
    Seconds abs_t;
    bool is_backing;
    uint32_t cycle_index;
};

struct TransitionWarping {
    uint32_t loops;
    bool back_and_forth;

    TransitionWarping(uint32_t loops = 1, bool back_and_forth = false);

    double duration_factor() const;
    TransitionTimePoint warp_time(
        const TransitionTimePoint& tp, const Seconds internal_duration) const;
};

class NodeTransitionBase {
  public:
    Seconds duration;
    Seconds internal_duration;

    TransitionWarping warping;

    NodeTransitionBase();
    NodeTransitionBase(
        const Seconds duration,
        const TransitionWarping& warping = TransitionWarping());

    virtual std::unique_ptr<TransitionStateBase> prepare_state(
        NodePtr node) const;
    virtual void process_time_point(
        TransitionStateBase* state, NodePtr node,
        const TransitionTimePoint& tp) const = 0;
};

class NodeTransitionCustomizable : public NodeTransitionBase {
  public:
    NodeTransitionCustomizable();
    NodeTransitionCustomizable(
        const Seconds duration,
        const TransitionWarping& warping = TransitionWarping(),
        const Easing easing = Easing::none);

    virtual void process_time_point(
        TransitionStateBase* state, NodePtr node,
        const TransitionTimePoint& tp) const;
    virtual void evaluate(
        TransitionStateBase* state, NodePtr node, const double t) const = 0;

    Easing _easing;
};

class NodeTransitionsGroupBase : public NodeTransitionBase {
    struct _SubTransition {
        NodeTransitionHandle handle;
        Seconds starting_time;
        Seconds ending_time;

        _SubTransition(
            const NodeTransitionHandle& handle, const Seconds starting_time,
            const Seconds ending_time)
            : handle(handle), starting_time(starting_time),
              ending_time(ending_time)
        {}
    };

  protected:
    NodeTransitionsGroupBase(
        const std::vector<NodeTransitionHandle>& transitions) noexcept(false);
    std::vector<_SubTransition> _sub_transitions;
    bool has_infinite_sub_transitions;
};

class NodeTransitionsSequence : public NodeTransitionsGroupBase {
  public:
    NodeTransitionsSequence(
        const std::vector<NodeTransitionHandle>& transitions,
        const TransitionWarping& warping = TransitionWarping()) noexcept(false);
    std::unique_ptr<TransitionStateBase> prepare_state(NodePtr node) const;
    void process_time_point(
        TransitionStateBase* state, NodePtr node,
        const TransitionTimePoint& tp) const;
};

class NodeTransitionsParallel : public NodeTransitionsGroupBase {
  public:
    NodeTransitionsParallel(
        const std::vector<NodeTransitionHandle>& transitions,
        const TransitionWarping& warping = TransitionWarping()) noexcept(false);
    std::unique_ptr<TransitionStateBase> prepare_state(NodePtr node) const;
    void process_time_point(
        TransitionStateBase* state, NodePtr node,
        const TransitionTimePoint& tp) const;
};

class NodeTransitionDelay : public NodeTransitionBase {
  public:
    NodeTransitionDelay(const Seconds duration);
    void process_time_point(
        TransitionStateBase* state, NodePtr node,
        const TransitionTimePoint& tp) const;
};

typedef std::function<void(NodePtr)> NodeTransitionCallbackFunc;

class NodeTransitionCallback : public NodeTransitionBase {
    NodeTransitionCallbackFunc callback_func;

  public:
    NodeTransitionCallback(const NodeTransitionCallbackFunc& func);
    void process_time_point(
        TransitionStateBase* state, NodePtr node,
        const TransitionTimePoint& tp) const;
};

struct NodeTransitionRunner {
    NodeTransitionHandle transition_handle;
    std::unique_ptr<TransitionStateBase> transition_state;
    bool transition_state_prepared = false;
    Microseconds current_time = 0us;

    NodeTransitionRunner(const NodeTransitionHandle& transition);
    ~NodeTransitionRunner() = default;
    NodeTransitionRunner(const NodeTransitionRunner&) = delete;
    NodeTransitionRunner(NodeTransitionRunner&&) = delete;

    NodeTransitionRunner& operator=(const NodeTransitionRunner&) = delete;
    NodeTransitionRunner& operator=(NodeTransitionRunner&&) = delete;

    NodeTransitionRunner& operator=(const NodeTransitionHandle& transition);

    void setup(const NodeTransitionHandle& transition);
    bool step(NodePtr node, const Microseconds dt);
    operator bool() const;
};

class NodeTransitionsManager {
    friend class Scene;

    std::unordered_map<std::string, NodeTransitionRunner> _transitions_map;
    std::vector<std::pair<std::string, NodeTransitionHandle>> _enqueued_updates;
    bool _is_processing = false;

    void step(NodePtr node, const Microseconds dt);

  public:
    NodeTransitionHandle get(const std::string& name);
    void set(const std::string& name, const NodeTransitionHandle& transition);
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
