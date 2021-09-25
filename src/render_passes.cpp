#include <array>

#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/render_passes.h"
#include "kaacore/shaders.h"
#include "kaacore/shapes.h"

namespace kaacore {

ResourceReference<Shader> Effect::_vertex_shader;
Shape Effect::_quad = Shape::Box(glm::dvec2(2., 2.));
ResourcesRegistry<EffectID, Effect> _effects_registry;

void
initialize_effects()
{
    // this cannot be done from global scope, crmc lib is not yet ready.
    Effect::_initialize_default_vertex_shader();
    _effects_registry.initialze();
}

void
uninitialize_effects()
{
    Effect::_uninitialize_default_vertex_shader();
}

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

Effect::Effect(
    const ResourceReference<Shader>& fragment_shader,
    const UniformSpecificationMap& uniforms)
    : _fragment_shader(fragment_shader), _uniform_specification(uniforms)
{
    KAACORE_CHECK(
        fragment_shader->type() == ShaderType::fragment,
        "Fragment shader is expected, but vertex shader was provided.");

    if (is_engine_initialized()) {
        this->_initialize();
    }
}

ResourceReference<Effect>
Effect::create(
    const ResourceReference<Shader>& fragment_shader,
    const UniformSpecificationMap& uniforms)
{
    auto id = Effect::_last_id.fetch_add(1, std::memory_order_relaxed);
    auto effect =
        std::shared_ptr<Effect>(new Effect(fragment_shader, uniforms));
    _effects_registry.register_resource(id, effect);
    return effect;
}

DrawCall
Effect::draw_call()
{
    RenderState state{nullptr, this->_material.get_valid(), 0, 0};
    // uint16_t
    return DrawCall::create(
        state, -1, this->_quad.vertices, this->_quad.indices);
}

void
Effect::_initialize()
{
    this->_material = std::move(Material::create(
        Program::create(this->_vertex_shader, this->_fragment_shader),
        this->_uniform_specification));
}

void
Effect::_uninitialize()
{}

void
Effect::_initialize_default_vertex_shader()
{
    _vertex_shader = load_embedded_shader("vs_effect", ShaderType::fragment);
}

void
Effect::_uninitialize_default_vertex_shader()
{
    _vertex_shader->_uninitialize();
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
    this->_clear_flags |= ClearFlag::color;
    this->_requires_clean = true;
}

void
RenderPass::reset_clear_color()
{
    this->_clear_flags &= ~ClearFlag::color;
    this->_requires_clean = true;
}

void
RenderPass::render_targets(
    const std::optional<std::vector<ResourceReference<RenderTarget>>>& targets)
{
    if (targets.has_value()) {
        auto value = targets.value();
        auto attachments_number = std::min(
            bgfx::getCaps()->limits.maxFBAttachments, max_attachments_number);
        KAACORE_CHECK(
            value.size() <= max_attachments_number,
            "The maximum supported number of render targets is {}.",
            max_attachments_number);
        this->_destroy_frame_buffer();
        this->_render_targets = value;
        std::array<bgfx::Attachment, max_attachments_number> attachments;
        for (int i; i < value.size(); ++i) {
            auto& render_target_ref = value[i];
            attachments[i].init(render_target_ref->_texture);
        }
        this->_frame_buffer = bgfx::createFrameBuffer(
            this->_render_targets.size(), attachments.data(), false);
    } else {
        this->_destroy_frame_buffer();
        this->_render_targets.clear();
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

RenderPassesManager::RenderPassesManager()
{
    for (auto pass_index = 0; pass_index < this->size(); ++pass_index) {
        this->_render_passes[pass_index]._index = pass_index;
        bgfx::setViewMode(pass_index, bgfx::ViewMode::DepthAscending);
    }
}

RenderPassStateArray
RenderPassesManager::take_snapshot()
{
    RenderPassStateArray result;
    for (auto& pass : *this) {

        bool has_targets = not pass._render_targets.empty();
        std::array<glm::dvec4, max_attachments_number> clear_colors;
        bool requires_clean = has_targets ? false : pass._requires_clean;
        if (not has_targets) {
            // if no render target is set, use pass._clear_color
            clear_colors[0] = pass._clear_color;
        } else {
            // in case render targets are provided, ignore pass._clear_color
            // and use render_target._clear_color
            for (int i = 0; i < pass._render_targets.size(); ++i) {
                auto& render_target = pass._render_targets[i];
                if (render_target->_requires_clean) {
                    requires_clean = true;
                }
                clear_colors[i] = render_target->_clear_color;
            }
        }

        result[pass._index] = {pass._index, requires_clean, pass._clear_flags,
                               pass._render_targets.size(), clear_colors};
    }
    return result;
}

RenderPass& RenderPassesManager::operator[](const int16_t z_index)
{
    KAACORE_CHECK(validate_render_pass_index(z_index), "Invalid view z_index.");
    auto index = z_index + (this->size() / 2);
    return this->_render_passes[index];
}

RenderPass*
RenderPassesManager::get(const int16_t z_index)
{
    return &this->operator[](z_index);
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
RenderPassesManager::_reset()
{
    for (auto& render_pass : *this) {
        render_pass._requires_clean = false;
        for (auto& render_target : render_pass._render_targets) {
            render_target->_requires_clean = false;
        }
    }
}

} // namespace kaacore
