#pragma once

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
    glm::uvec2 get_dimensions() const override;
    glm::dvec4 clear_color() const;
    void clear_color(const glm::dvec4& color);

  private:
    RenderTargetID _id;
    glm::dvec4 _clear_color;
    bool _is_dirty = false;

    RenderTarget(RenderTargetID id);
    virtual void _initialize() override;
    virtual void _uninitialize() override;
    void _mark_dirty();

    static inline std::atomic<RenderTargetID> _last_id = 0;

    friend class RenderPass;
    friend class RenderPassesManager;
    friend class ResourcesRegistry<RenderTargetID, RenderTarget>;
};

} // namespace kaacore
