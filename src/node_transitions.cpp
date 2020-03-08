#include <memory>

#include "kaacore/exceptions.h"
#include "kaacore/log.h"

#include "kaacore/node_transitions.h"

namespace kaacore {

struct _NodeSpriteAnimationTransitionState : TransitionStateBase {
    int32_t last_frame_index = -1;
};

NodeSpriteTransition::NodeSpriteTransition(
    const std::vector<Sprite>& frames, const double duration,
    const TransitionWarping& warping)
    : NodeTransitionCustomizable(duration, warping), _frames(frames),
      _frames_count(frames.size())
{
    KAACORE_CHECK(this->_frames_count > 0);
}

std::unique_ptr<TransitionStateBase>
NodeSpriteTransition::prepare_state(NodePtr node) const
{
    return std::make_unique<_NodeSpriteAnimationTransitionState>();
}

void
NodeSpriteTransition::evaluate(
    TransitionStateBase* state_b, NodePtr node, const double t) const
{
    auto state = static_cast<_NodeSpriteAnimationTransitionState*>(state_b);
    int32_t target_frame = (t * (this->_frames_count + 1));
    if (target_frame >= this->_frames_count) {
        target_frame = this->_frames_count - 1;
    }
    log<LogLevel::debug, LogCategory::misc>(
        "NodeSpriteTransition(%p)::evaluate - node: %p, t: "
        "%lf, frames_count: %lu, target_frame %lu",
        this, node.get(), t, this->_frames_count, target_frame);

    if (target_frame != state->last_frame_index) {
        state->last_frame_index = target_frame;
        node->sprite(this->_frames[target_frame]);
    };
}

} // namespace kaacore
