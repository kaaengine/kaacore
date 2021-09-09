#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/render_passes.h"

namespace kaacore {

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

RenderPass::RenderPass() : _requires_clean(false) {}

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

RenderPassesManager::RenderPassesManager()
{
    for (uint16_t view_index = 0; view_index < this->size(); ++view_index) {
        this->_render_passes[view_index]._index = view_index;
    }
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

} // namespace kaacore
