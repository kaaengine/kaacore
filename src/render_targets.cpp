#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/render_targets.h"

namespace kaacore {

class RenderTargetsRegistry
    : public ResourcesRegistry<RenderTargetId, RenderTarget> {
    friend void reset_render_targets(const glm::uvec2&);
};

class FrameBuffersRegistry
    : public ResourcesRegistry<FrameBufferId, FrameBuffer> {
    friend void reset_render_targets(const glm::uvec2&);
};

RenderTargetsRegistry _render_targets_registry;
FrameBuffersRegistry _frame_buffers_registry;

void
initialize_render_targets()
{
    _render_targets_registry.initialze();
    _frame_buffers_registry.initialze();
}

void
uninitialize_render_targets()
{
    _render_targets_registry.uninitialze();
    _frame_buffers_registry.uninitialze();
}

void
reset_render_targets(const glm::uvec2& size)
{
    RenderTarget::_dimensions = size;
    for (auto& it : _render_targets_registry._registry) {
        auto resource = it.second.lock();
        if (resource and resource->is_initialized) {
            std::static_pointer_cast<RenderTarget>(resource)->_reset();
        }
    }
    for (auto& it : _frame_buffers_registry._registry) {
        auto resource = it.second.lock();
        if (resource and resource->is_initialized) {
            std::static_pointer_cast<FrameBuffer>(resource)->_reset();
        }
    }
}

RenderTarget::RenderTarget(RenderTargetId id) : _id(id)
{
    if (is_engine_initialized()) {
        this->_initialize();
    }
}

RenderTarget::~RenderTarget()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<RenderTarget>
RenderTarget::create()
{
    auto id = RenderTarget::_last_id.fetch_add(1, std::memory_order_relaxed);
    auto render_target = std::shared_ptr<RenderTarget>(new RenderTarget(id));
    _render_targets_registry.register_resource(id, render_target);
    return render_target;
}

glm::uvec2
RenderTarget::get_dimensions() const
{
    return this->_dimensions;
}

void
RenderTarget::_reset()
{
    if (bgfx::isValid(this->_handle)) {
        bgfx::destroy(this->_handle);
    }
    auto dimensions = glm::max({1, 1}, this->_dimensions);
    this->_handle = this->_create_texture(dimensions);
}

void
RenderTarget::_initialize()
{
    this->_handle = this->_create_texture(this->_dimensions);
    this->is_initialized = true;
}

bgfx::TextureHandle
RenderTarget::_create_texture(const glm::uvec2& dimensions)
{
    auto handle = bgfx::createTexture2D(
        dimensions.x, dimensions.y, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_RT);
    KAACORE_CHECK(
        bgfx::isValid(handle), "Failed to create render target texture.");
    bgfx::setName(
        handle,
        fmt::format("Texture for RenderTarget({})", handle.idx).c_str());
    return handle;
}

void
RenderTarget::_uninitialize()
{
    bgfx::destroy(this->_handle);
    this->is_initialized = false;
}

FrameBuffer::FrameBuffer(
    const FrameBufferId id,
    const std::vector<ResourceReference<RenderTarget>>& targets)
    : _id(id), _render_targets(targets)
{

    if (is_engine_initialized()) {
        this->_initialize();
    }
}

FrameBuffer::~FrameBuffer()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

ResourceReference<FrameBuffer>
FrameBuffer::create(const std::vector<ResourceReference<RenderTarget>>& targets)
{
    auto max_attachments = bgfx::getCaps()->limits.maxFBAttachments;
    KAACORE_CHECK(
        targets.size() <= max_attachments,
        "The maximum supported number of render targets is {}.",
        max_attachments);

    auto id = FrameBuffer::_last_id.fetch_add(1, std::memory_order_relaxed);
    auto frame_buffer =
        std::shared_ptr<FrameBuffer>(new FrameBuffer(id, targets));
    _frame_buffers_registry.register_resource(id, frame_buffer);
    return frame_buffer;
}

std::vector<ResourceReference<RenderTarget>>&
FrameBuffer::render_targets()
{
    return this->_render_targets;
}

void
FrameBuffer::_reset()
{
    if (bgfx::isValid(this->_handle)) {
        bgfx::destroy(this->_handle);
    }
    this->_handle = this->_create_frame_buffer();
}

void
FrameBuffer::_initialize()
{
    this->_handle = this->_create_frame_buffer();
    this->is_initialized = true;
}

bgfx::FrameBufferHandle
FrameBuffer::_create_frame_buffer()
{
    std::vector<bgfx::Attachment> attachments(this->_render_targets.size());
    for (auto i = 0; i < this->_render_targets.size(); ++i) {
        auto& render_target = this->_render_targets[i];
        attachments[i].init(render_target->_handle);
    }
    auto handle = bgfx::createFrameBuffer(
        this->_render_targets.size(), attachments.data(), false);
    KAACORE_CHECK(bgfx::isValid(handle), "Failed to create frame buffer.");
    return handle;
}

void
FrameBuffer::_uninitialize()
{
    if (bgfx::isValid(this->_handle)) {
        bgfx::destroy(this->_handle);
        this->_handle = backbuffer_handle;
    }
}

} // namespace kaacore
