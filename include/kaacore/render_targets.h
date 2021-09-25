#pragma once

#include <optional>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/resources.h"
#include "kaacore/textures.h"

namespace kaacore {

using RenderTargetID = uint32_t;

void
initialize_render_targets();

void
uninitialize_render_targets();

class RenderPass;
class RenderPassesManager;

class RenderTarget : public Texture {
  public:
    ~RenderTarget();
    static ResourceReference<RenderTarget> create();
    glm::uvec2 get_dimensions() override;

  private:
    RenderTargetID _id;
    bool _requires_clean;
    glm::dvec4 _clear_color;
    bgfx::TextureHandle _texture;

    RenderTarget();
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    static inline std::atomic<RenderTargetID> _last_id = 0;

    friend class RenderPass;
    friend class RenderPassesManager;
    friend class ResourcesRegistry<RenderTargetID, RenderTarget>;
};

} // namespace kaacore
