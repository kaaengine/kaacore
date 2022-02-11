#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/resources.h"
#include "kaacore/textures.h"

namespace kaacore {

using FrameBufferId = uint32_t;
using RenderTargetId = uint32_t;
// setViewClear has only 8 slots for attachment clear values
constexpr auto max_attachments_number = 8u;
constexpr bgfx::FrameBufferHandle backbuffer_handle = BGFX_INVALID_HANDLE;

void
initialize_render_targets();

void
uninitialize_render_targets();

void
reset_render_targets(const glm::uvec2& size);

class FrameBuffer;

class RenderTarget : public Texture {
  public:
    ~RenderTarget();
    static ResourceReference<RenderTarget> create();
    glm::uvec2 get_dimensions() const override;
    glm::dvec4 clear_color() const;
    void clear_color(const glm::dvec4& value);

  private:
    RenderTargetId _id;
    bool _is_dirty = false;
    glm::dvec4 _clear_color = {0, 0, 0, 0};

    RenderTarget(RenderTargetId id);
    void _reset();
    bgfx::TextureHandle _create_texture(const glm::uvec2& size);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    static inline glm::uvec2 _dimensions;
    static inline std::atomic<RenderTargetId> _last_id = 0;

    friend class FrameBuffer;
    friend class ResourcesRegistry<RenderTargetId, RenderTarget>;
    friend void reset_render_targets(const glm::uvec2& dimensions);
};

class RenderPass;

struct FrameBufferState {
    bool requires_clear;
    size_t active_attachments_number;
    std::array<glm::dvec4, max_attachments_number> clear_colors;
};

class FrameBuffer : public Resource {
  public:
    FrameBuffer() = default;
    ~FrameBuffer();

    static ResourceReference<FrameBuffer> create(
        const std::vector<ResourceReference<RenderTarget>>& targets);
    std::vector<ResourceReference<RenderTarget>>& render_targets();

  private:
    FrameBufferId _id;
    bgfx::FrameBufferHandle _handle = backbuffer_handle;
    static inline std::atomic<FrameBufferId> _last_id = 0;
    std::vector<ResourceReference<RenderTarget>> _render_targets;

    FrameBuffer(
        const FrameBufferId id,
        const std::vector<ResourceReference<RenderTarget>>& targets);
    void _reset();
    FrameBufferState _take_snapshot();
    bgfx::FrameBufferHandle _create_frame_buffer();
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    friend class RenderPass;
    friend class ResourcesRegistry<FrameBufferId, FrameBuffer>;
    friend void reset_render_targets(const glm::uvec2& dimensions);
};

} // namespace kaacore
