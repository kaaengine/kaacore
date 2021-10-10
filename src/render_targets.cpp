#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/render_targets.h"

namespace kaacore {

ResourcesRegistry<RenderTargetID, RenderTarget> _render_targets_registry;

void
initialize_render_targets()
{
    _render_targets_registry.initialze();
}

void
uninitialize_render_targets()
{
    _render_targets_registry.uninitialze();
}

RenderTarget::RenderTarget(RenderTargetID id) : _id(id)
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
    return get_engine()->window->size();
}

glm::dvec4
RenderTarget::clear_color() const
{
    return this->_clear_color;
}

void
RenderTarget::clear_color(const glm::dvec4& color)
{
    this->_clear_color = color;
    this->_is_dirty = true;
}

void
RenderTarget::_initialize()
{
    this->_handle = bgfx::createTexture2D(
        bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_RT);

    KAACORE_CHECK(
        bgfx::isValid(this->_handle),
        "Failed to create render target texture.");
    bgfx::setName(
        this->_handle,
        fmt::format("Texture for RenderTarget({})", this->_id).c_str());
    this->is_initialized = true;
}

void
RenderTarget::_uninitialize()
{
    bgfx::destroy(this->_handle);
    this->is_initialized = false;
}

void
RenderTarget::_mark_dirty()
{
    this->_is_dirty = true;
}

} // namespace kaacore
