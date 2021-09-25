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

RenderTarget::RenderTarget()
{
    this->_id = this->_last_id.fetch_add(1, std::memory_order_relaxed);
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
    auto render_target = std::shared_ptr<RenderTarget>(new RenderTarget);
    _render_targets_registry.register_resource(id, render_target);
    return render_target;
}

glm::uvec2
RenderTarget::get_dimensions()
{
    return get_engine()->window->size();
}

void
RenderTarget::_initialize()
{
    this->_texture = bgfx::createTexture2D(
        bgfx::BackbufferRatio::Equal, false, 1, bgfx::TextureFormat::BGRA8,
        BGFX_TEXTURE_RT);
    KAACORE_CHECK(
        bgfx::isValid(this->_texture),
        "Failed to create render target texture.");
}

void
RenderTarget::_uninitialize()
{
    bgfx::destroy(this->_texture);
    this->is_initialized = false;
}

} // namespace kaacore
