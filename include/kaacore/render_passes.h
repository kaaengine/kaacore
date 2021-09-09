#pragma once

#include <bitset>
#include <unordered_set>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/camera.h"
#include "kaacore/config.h"
#include "kaacore/indexset.h"

namespace kaacore {

constexpr auto min_pass_index = 0;
constexpr auto max_pass_index = KAACORE_MAX_RENDER_PASSES - 1;
constexpr auto default_pass_index = min_pass_index;

inline bool
validate_render_pass_index(const int16_t render_pass_index)
{
    return (min_pass_index <= render_pass_index) and
           (render_pass_index <= max_pass_index);
}

enum class ClearFlag : uint16_t {
    none = BGFX_CLEAR_NONE,
    color = BGFX_CLEAR_COLOR,
    depth = BGFX_CLEAR_DEPTH,
    stencil = BGFX_CLEAR_STENCIL,
    discard_color0 = BGFX_CLEAR_DISCARD_COLOR_0,
    discard_color1 = BGFX_CLEAR_DISCARD_COLOR_1,
    discard_color2 = BGFX_CLEAR_DISCARD_COLOR_2,
    discard_color3 = BGFX_CLEAR_DISCARD_COLOR_3,
    discard_color4 = BGFX_CLEAR_DISCARD_COLOR_4,
    discard_color5 = BGFX_CLEAR_DISCARD_COLOR_5,
    discard_color6 = BGFX_CLEAR_DISCARD_COLOR_6,
    discard_color7 = BGFX_CLEAR_DISCARD_COLOR_7,
    discard_depth = BGFX_CLEAR_DISCARD_DEPTH,
    discard_stencil = BGFX_CLEAR_DISCARD_STENCIL,
    discard_color_mask = BGFX_CLEAR_DISCARD_COLOR_MASK,
    discard_mask = BGFX_CLEAR_DISCARD_MASK
};

uint16_t
operator~(ClearFlag flag);
uint16_t
operator|(ClearFlag left, ClearFlag right);
uint16_t
operator|(ClearFlag left, uint16_t right);
uint16_t
operator|(uint16_t left, ClearFlag right);
uint16_t
operator|=(uint16_t& left, ClearFlag right);

using RenderPassIndexSet = IndexSet<KAACORE_MAX_RENDER_PASSES>;

class Renderer;
class RenderPassesManager;

class RenderPass {
  public:
    Camera camera;

    RenderPass& operator=(const RenderPass&) = delete;

    uint16_t index() const;
    glm::dvec4 clear_color() const;
    void clear_color(const glm::dvec4& color);
    void reset_clear_color();

  private:
    uint16_t _index;
    bool _requires_clean;
    glm::dvec4 _clear_color;
    uint16_t _clear_flags =
        ClearFlag::none | ClearFlag::depth | ClearFlag::color;

    RenderPass();

    friend class Renderer;
    friend class RenderPassesManager;
};

class Scene;

class RenderPassesManager {
  public:
    RenderPassesManager();
    RenderPass& operator[](const int16_t z_index);
    RenderPass* get(const int16_t z_index);
    RenderPass* begin();
    RenderPass* end();

    size_t size();

  private:
    RenderPass _render_passes[KAACORE_MAX_RENDER_PASSES];

    friend class Scene;
};

} // namespace kaacore

namespace std {
template<>
struct hash<kaacore::RenderPassIndexSet> {
    size_t operator()(const kaacore::RenderPassIndexSet& render_passes) const
    {
        return std::hash<std::bitset<KAACORE_MAX_RENDER_PASSES>>{}(
            render_passes._bitset);
    }
};
}
