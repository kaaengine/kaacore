#pragma once

#include <array>
#include <bitset>
#include <optional>
#include <unordered_set>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/config.h"
#include "kaacore/indexset.h"
#include "kaacore/materials.h"
#include "kaacore/render_targets.h"
#include "kaacore/resources.h"
#include "kaacore/vertex_layout.h"

namespace kaacore {

constexpr auto min_pass_index = 0;
constexpr auto max_pass_index = KAACORE_MAX_RENDER_PASSES - 1;
constexpr auto default_pass_index = min_pass_index;

struct RenderPassState;
using RenderPassStateArray =
    std::array<RenderPassState, KAACORE_MAX_RENDER_PASSES>;
using EffectID = uint32_t;
using RenderPassIndexSet = IndexSet<KAACORE_MAX_RENDER_PASSES>;

inline bool
validate_render_pass_index(const uint16_t render_pass_index)
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

class Shape;
class DrawCall;

struct Quad {
    std::vector<VertexIndex> indices;
    std::vector<StandardVertexData> vertices;

    Quad();
};

class Effect {
  public:
    Effect() = default;
    Effect(
        const ResourceReference<Shader>& fragment_shader,
        const UniformSpecificationMap& uniforms = {});
    bool operator==(const Effect& other);
    ResourceReference<Material>& material();
    Effect clone();
    DrawCall draw_call() const;

  private:
    static Quad _quad;
    ResourceReference<Material> _material;

    friend std::hash<Effect>;
};

struct RenderPassState {
    uint16_t index;
    bool requires_clean;
    uint16_t clear_flags;
    glm::dvec4 clear_color;
    bgfx::FrameBufferHandle frame_buffer;

    inline bool has_custom_framebuffer() const
    {
        return bgfx::isValid(this->frame_buffer);
    }
};

class RenderPassesManager;

class RenderPass {
    using RenderTargets = std::vector<ResourceReference<RenderTarget>>;

  public:
    RenderPass& operator=(const RenderPass&) = delete;
    uint16_t index() const;
    glm::dvec4 clear_color() const;
    void clear_color(const glm::dvec4& color);
    std::optional<Effect> effect();
    void effect(const std::optional<Effect>& effect);
    void render_targets(const std::optional<RenderTargets>& targets);
    std::optional<RenderTargets> render_targets();

  private:
    bool _is_dirty;
    uint16_t _index;
    glm::dvec4 _clear_color = {0, 0, 0, 0};
    uint16_t _clear_flags = ClearFlag::none | ClearFlag::depth |
                            ClearFlag::color | ClearFlag::stencil;
    std::optional<Effect> _effect;
    ResourceReference<FrameBuffer> _frame_buffer;

    RenderPass();
    void _mark_dirty();
    RenderPassState _take_snapshot();

    friend class RenderPassesManager;
};

class Scene;

class RenderPassesManager {
  public:
    RenderPassesManager();
    RenderPass& operator[](const uint16_t index);
    RenderPass* get(const uint16_t index);
    RenderPass* begin();
    RenderPass* end();
    size_t size();

  private:
    RenderPass _render_passes[KAACORE_MAX_RENDER_PASSES];

    void _mark_dirty();
    RenderPassStateArray _take_snapshot();

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

template<>
struct hash<kaacore::Effect> {
    size_t operator()(const kaacore::Effect& effect) const
    {
        return std::hash<kaacore::ResourceReference<kaacore::Material>>{}(
            effect._material);
    }
};
}
