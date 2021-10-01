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

namespace kaacore {

constexpr auto min_pass_index = 0;
constexpr auto max_pass_index = KAACORE_MAX_RENDER_PASSES - 1;
constexpr auto default_pass_index = min_pass_index;
constexpr bgfx::FrameBufferHandle backbuffer_handle = BGFX_INVALID_HANDLE;
// setViewClear has only 8 slots for attachment clear values
constexpr auto max_attachments_number = 8u;

struct RenderPassState;
using RenderPassStateArray =
    std::array<RenderPassState, KAACORE_MAX_RENDER_PASSES>;
using EffectID = uint32_t;
using RenderPassIndexSet = IndexSet<KAACORE_MAX_RENDER_PASSES>;

void
initialize_effects();

void
uninitialize_effects();

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

class Effect : public Resource {
  public:
    static ResourceReference<Effect> create(
        const ResourceReference<Shader>& fragment_shader,
        const UniformSpecificationMap& uniforms = {});
    ResourceReference<Material>& material();
    DrawCall draw_call();

  private:
    ResourceReference<Material> _material;
    ResourceReference<Shader> _fragment_shader;
    UniformSpecificationMap _uniform_specification;

    static Shape _quad;
    static ResourceReference<Shader> _vertex_shader;
    static inline std::atomic<EffectID> _last_id = 0;

    Effect(
        const ResourceReference<Shader>& fragment_shader,
        const UniformSpecificationMap& uniforms);

    void _initialize() override;
    void _uninitialize() override;

    static void _initialize_default_vertex_shader();
    static void _uninitialize_default_vertex_shader();

    friend void initialize_effects();
    friend void uninitialize_effects();
    friend class ResourcesRegistry<EffectID, Effect>;
};

class Renderer;
class RenderPassesManager;

class RenderPass {
  public:
    ResourceReference<Effect> effect;

    ~RenderPass();
    RenderPass& operator=(const RenderPass&) = delete;

    uint16_t index() const;
    glm::dvec4 clear_color() const;
    void clear_color(const glm::dvec4& color);
    void render_targets(
        const std::optional<std::vector<ResourceReference<RenderTarget>>>&
            targets);
    std::optional<std::vector<ResourceReference<RenderTarget>>>
    render_targets();

  private:
    uint16_t _index;
    bool _requires_clean;
    glm::dvec4 _clear_color;
    uint16_t _clear_flags = ClearFlag::none | ClearFlag::depth |
                            ClearFlag::color | ClearFlag::stencil;
    bgfx::FrameBufferHandle _frame_buffer = backbuffer_handle;
    std::vector<ResourceReference<RenderTarget>> _render_targets;

    RenderPass();
    void _destroy_frame_buffer();

    friend class Renderer;
    friend class RenderPassesManager;
};

struct RenderPassState {
    uint16_t index;
    bool requires_clean;
    uint16_t clear_flags;
    size_t active_attachments_number;
    std::array<glm::dvec4, max_attachments_number> clear_colors;
};

class Renderer;

class RenderPassesManager {
  public:
    RenderPassesManager();
    RenderPassStateArray take_snapshot();
    RenderPass& operator[](const uint16_t index);
    RenderPass* get(const uint16_t index);
    RenderPass* begin();
    RenderPass* end();

    size_t size();

  private:
    RenderPass _render_passes[KAACORE_MAX_RENDER_PASSES];

    void _reset();

    friend class Renderer;
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
