#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>


namespace kaacore {

struct Node;

class NodeTransitionBase;
typedef std::shared_ptr<const NodeTransitionBase> NodeTransitionHandle;


struct TransitionStateBase {
};


class NodeTransitionBase {
    public:
    double duration;

    NodeTransitionBase();
    NodeTransitionBase(const double duration);

    virtual std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
    virtual void evaluate(TransitionStateBase* state, Node* node, const double t) const = 0;
};


class NodeTransitionsGroupBase : public NodeTransitionBase {
    struct _SubTransition {
        NodeTransitionHandle handle;
        double ending_time;

        _SubTransition(const NodeTransitionHandle& handle, const double ending_time)
        : handle(handle), ending_time(ending_time)
        {
        }
    };

    protected:
    std::vector<_SubTransition> _sub_transitions;

    public:
    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const;
};


class NodeTransitionsSequence : public NodeTransitionsGroupBase {
    public:
    NodeTransitionsSequence(const std::vector<NodeTransitionHandle>& transitions) noexcept(false);
    void evaluate(TransitionStateBase* state, Node* node, const double t) const;
};


class NodeTransitionsParallel : public NodeTransitionsGroupBase {
    public:
    NodeTransitionsParallel(const std::vector<NodeTransitionHandle>& transitions) noexcept(false);
    void evaluate(TransitionStateBase* state, Node* node, const double t) const;
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
NodeTransitionHandle make_node_transition(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
};

inline NodeTransitionHandle make_node_transitions_sequence(const std::vector<NodeTransitionHandle>& transitions) {
    return std::make_shared<NodeTransitionsSequence>(transitions);
}

inline NodeTransitionHandle make_node_transitions_parallel(const std::vector<NodeTransitionHandle>& transitions) {
    return std::make_shared<NodeTransitionsParallel>(transitions);
}

} // namespace kaacore
