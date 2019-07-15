#pragma once

#include <memory>

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

    NodeTransitionBase(const double duration);

    virtual std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const = 0;
    virtual void evaluate(TransitionStateBase* state, Node* node, const double t) const = 0;
};

class NodePositionTransition : public NodeTransitionBase {
    glm::dvec2 _move_vector;

    public:
    NodePositionTransition(const glm::dvec2& move_vector, const double duration);
    std::unique_ptr<TransitionStateBase> prepare_state(Node* node) const override;
    void evaluate(TransitionStateBase* state, Node* node, const double t) const override;
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


} // namespace kaacore
