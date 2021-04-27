#pragma once

#include <memory>
#include <vector>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "kaacore/draw_queue.h"
#include "kaacore/draw_unit.h"
#include "kaacore/files.h"
#include "kaacore/images.h"
#include "kaacore/log.h"
#include "kaacore/resources.h"
#include "kaacore/shaders.h"
#include "kaacore/utils.h"
#include "kaacore/views.h"

namespace kaacore {

class Renderer {
  public:
    bgfx::VertexLayout vertex_layout;

    std::unique_ptr<Image> default_image;

    bgfx::UniformHandle texture_uniform;
    ResourceReference<Program> default_program;
    ResourceReference<Program> sdf_font_program;
    // TODO replace with default_image
    bgfx::TextureHandle default_texture;

    glm::uvec2 view_size;
    glm::uvec2 border_size;
    uint32_t border_color = 0x000000ff;

    Renderer(bgfx::Init bgfx_init_data, const glm::uvec2& window_size);
    ~Renderer();

    bgfx::TextureHandle make_texture(
        std::shared_ptr<bimg::ImageContainer> image_container,
        const uint64_t flags) const;
    void destroy_texture(const bgfx::TextureHandle& handle) const;
    void begin_frame();
    void end_frame();
    void push_statistics() const;
    void reset();
    void process_view(View& view) const;
    void render_vertices(
        const uint16_t view_index,
        const std::vector<StandardVertexData>& vertices,
        const std::vector<VertexIndex>& indices,
        const bgfx::TextureHandle texture,
        const ResourceReference<Program>& program) const;
    void render_draw_unit(const DrawBucketKey& key, const DrawUnit& draw_unit);
    void render_draw_bucket(
        const DrawBucketKey& key, const DrawBucket& draw_bucket);
    void render_draw_queue(const DrawQueue& draw_queue);

  private:
    void _submit_draw_bucket_state(const DrawBucketKey& key);
    uint32_t _calculate_reset_flags() const;

    bool _vertical_sync = true;

    friend class Engine;
};

} // namespace kaacore
