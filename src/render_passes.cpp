#include <array>

#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/render_passes.h"
#include "kaacore/shaders.h"
#include "kaacore/shapes.h"

namespace kaacore {

Quad Effect::_quad;
ResourcesRegistry<EffectID, Effect> _effects_registry;

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

RenderPass::RenderPass() : _requires_clean(false) {}

RenderPass::~RenderPass()
{
    this->_destroy_frame_buffer();
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
    this->_requires_clean = true;
}

void
RenderPass::render_targets(
    const std::vector<ResourceReference<RenderTarget>>& targets)
{
    auto attachments_number = std::min(
        bgfx::getCaps()->limits.maxFBAttachments, max_attachments_number);
    KAACORE_CHECK(
        targets.size() <= max_attachments_number,
        "The maximum supported number of render targets is {}.",
        max_attachments_number);

    this->_render_targets = targets;
    this->_reset_frame_buffer();
}

std::vector<ResourceReference<RenderTarget>>&
RenderPass::render_targets()
{
    return this->_render_targets;
}

void
RenderPass::_create_frame_buffer()
{
    if (this->_render_targets.size()) {
        std::array<bgfx::Attachment, max_attachments_number> attachments;
        for (auto i = 0; i < this->_render_targets.size(); ++i) {
            auto& render_target = this->_render_targets[i];
            attachments[i].init(render_target->_handle);
        }
        this->_frame_buffer = bgfx::createFrameBuffer(
            this->_render_targets.size(), attachments.data(), false);
    }
}

void
RenderPass::_destroy_frame_buffer()
{
    if (bgfx::isValid(this->_frame_buffer)) {
        bgfx::destroy(this->_frame_buffer);
        this->_frame_buffer = backbuffer_handle;
    }
}

void
RenderPass::_reset_frame_buffer()
{
    this->_destroy_frame_buffer();
    this->_create_frame_buffer();
}

RenderPassState
RenderPass::_take_snapshot() const
{
    std::array<glm::dvec4, max_attachments_number> clear_colors;
    bool has_targets = not this->_render_targets.empty();
    bool requires_clean = has_targets ? false : this->_requires_clean;
    if (not has_targets) {
        // if no render target is set, use pass._clear_color
        clear_colors[0] = this->_clear_color;
    } else {
        // in case render targets are provided, ignore pass._clear_color
        // and use render_target._clear_color
        for (int i = 0; i < this->_render_targets.size(); ++i) {
            auto& render_target = this->_render_targets[i];
            if (render_target->_is_dirty) {
                requires_clean = true;
                render_target->_is_dirty = false;
            }
            clear_colors[i] = render_target->_clear_color;
        }
    }
    return {this->_index,        requires_clean,
            this->_clear_flags,  this->_render_targets.size(),
            this->_frame_buffer, clear_colors};
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
    for (auto& render_pass : *this) {
        for (auto& target : render_pass.render_targets()) {
            target->_mark_dirty();
        }
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
