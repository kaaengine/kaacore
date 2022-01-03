#include <array>

#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/render_passes.h"
#include "kaacore/shaders.h"
#include "kaacore/shapes.h"

namespace kaacore {

Quad Effect::_quad;
ResourcesRegistry<EffectId, Effect> _effects_registry;

uint16_t
operator~(ClearFlag flag)
{
    return ~static_cast<uint16_t>(flag);
}

uint16_t
operator|(ClearFlag left, ClearFlag right)
{
    return static_cast<uint16_t>(left) | static_cast<uint16_t>(right);
}

uint16_t
operator|(ClearFlag left, uint16_t right)
{
    return static_cast<uint16_t>(left) | right;
}

uint16_t
operator|(uint16_t left, ClearFlag right)
{
    return left | static_cast<uint16_t>(right);
}

uint16_t
operator|=(uint16_t& left, ClearFlag right)
{
    return left = left | static_cast<uint16_t>(right);
}

Quad::Quad()
{
    this->vertices = {StandardVertexData::xy_uv(-1., +1., 0., 0.),
                      StandardVertexData::xy_uv(+1., +1., 1., 0.),
                      StandardVertexData::xy_uv(+1., -1., 1., 1.),
                      StandardVertexData::xy_uv(-1., -1., 0., 1.)};
    this->indices = {0, 2, 1, 0, 3, 2};
}

Effect::Effect(
    const ResourceReference<Shader>& fragment_shader,
    const UniformSpecificationMap& uniforms)
{
    KAACORE_CHECK(
        fragment_shader->type() == ShaderType::fragment,
        "Fragment shader is expected.");

    this->_material = std::move(Material::create(
        Program::create(
            EmbeddedShader::load(ShaderType::vertex, "vs_effect"),
            fragment_shader),
        uniforms));
}

ResourceReference<Material>&
Effect::material()
{
    return this->_material;
}

bool
Effect::operator==(const Effect& other)
{
    return this->_material = other._material;
}

Effect
Effect::clone()
{
    Effect result;
    result._material = this->_material->clone();
    return result;
}

DrawCall
Effect::draw_call() const
{
    RenderState state{nullptr, this->_material.get_valid(), 0, 0};
    return DrawCall::create(
        state, -1, this->_quad.vertices, this->_quad.indices);
}

uint16_t
RenderPass::index() const
{
    return this->_index;
}

glm::dvec4
RenderPass::clear_color() const
{
    return this->_clear_color;
}

void
RenderPass::clear_color(const glm::dvec4& color)
{
    this->_clear_color = color;
    this->_mark_dirty();
}

std::optional<Effect>
RenderPass::effect()
{
    return this->_effect;
}

void

RenderPass::effect(const std::optional<Effect>& effect)
{
    this->_effect = effect;
}

void
RenderPass::render_targets(const std::optional<RenderTargets>& targets)
{
    if (targets.has_value()) {
        this->_frame_buffer = FrameBuffer::create(*targets);
    } else {
        this->_frame_buffer = ResourceReference<FrameBuffer>();
    }
}

std::optional<RenderPass::RenderTargets>
RenderPass::render_targets()
{
    if (this->_frame_buffer) {
        return this->_frame_buffer->_render_targets;
    }
    return std::nullopt;
}

void
RenderPass::_mark_dirty()
{
    this->_is_dirty = true;
}

RenderPassState
RenderPass::_take_snapshot()
{
    RenderPassState result;
    result.index = this->_index;
    result.requires_clear = this->_is_dirty;
    result.clear_flags = this->_clear_flags;
    result.clear_color = this->_clear_color;
    this->_is_dirty = false;

    if (this->_frame_buffer) {
        result.frame_buffer = this->_frame_buffer->_handle;
    } else {
        result.frame_buffer = backbuffer_handle;
    }

    return result;
}

RenderPassesManager::RenderPassesManager()
{
    for (auto pass_index = 0; pass_index < this->size(); ++pass_index) {
        this->_render_passes[pass_index]._index = pass_index;
    }
}

RenderPass& RenderPassesManager::operator[](const uint16_t index)
{
    KAACORE_CHECK(
        validate_render_pass_index(index), "Invalid render pass index.");
    return this->_render_passes[index];
}

RenderPass*
RenderPassesManager::get(const uint16_t index)
{
    return &this->operator[](index);
}

RenderPass*
RenderPassesManager::begin()
{
    return this->_render_passes;
}

RenderPass*
RenderPassesManager::end()
{
    return &this->_render_passes[this->size()];
}

size_t
RenderPassesManager::size()
{
    return KAACORE_MAX_RENDER_PASSES;
}

void
RenderPassesManager::_mark_dirty()
{
    for (auto& pass : *this) {
        pass._mark_dirty();
    }
}

RenderPassStateArray
RenderPassesManager::_take_snapshot()
{
    RenderPassStateArray result;
    for (auto& pass : *this) {
        result[pass._index] = pass._take_snapshot();
    }
    return result;
}

} // namespace kaacore
