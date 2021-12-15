#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/resources.h"
#include "kaacore/textures.h"

namespace kaacore {

using FrameBufferID = uint32_t;
using RenderTargetID = uint32_t;
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

  private:
    RenderTargetID _id;

    RenderTarget(RenderTargetID id);
    void _reset();
    bgfx::TextureHandle _create_texture(const glm::uvec2& size);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    static inline glm::uvec2 _dimensions;
    static inline std::atomic<RenderTargetID> _last_id = 0;

    friend class FrameBuffer;
    friend class ResourcesRegistry<RenderTargetID, RenderTarget>;
    friend void reset_render_targets(const glm::uvec2& dimensions);
};

class RenderPass;

class FrameBuffer : public Resource {
  public:
    FrameBuffer() = default;
    ~FrameBuffer();

    static ResourceReference<FrameBuffer> create(
        const std::vector<ResourceReference<RenderTarget>>& targets);
    std::vector<ResourceReference<RenderTarget>>& render_targets();

  private:
    bool _is_dirty;
    FrameBufferID _id;
    bgfx::FrameBufferHandle _handle = backbuffer_handle;
    static inline std::atomic<FrameBufferID> _last_id = 0;
    std::vector<ResourceReference<RenderTarget>> _render_targets;

    FrameBuffer(
        const FrameBufferID id,
        const std::vector<ResourceReference<RenderTarget>>& targets);
    void _reset();
    void _mark_dirty();
    bgfx::FrameBufferHandle _create_frame_buffer();
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    friend class RenderPass;
    friend class ResourcesRegistry<FrameBufferID, FrameBuffer>;
    friend void reset_render_targets(const glm::uvec2& dimensions);
};

} // namespace kaacore
