#pragma once

#include <string>
#include <unordered_set>

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "../third_party/bgfx/bgfx/src/config.h"
#include "kaacore/camera.h"

namespace kaacore {

#define KAACORE_MAX_VIEWS BGFX_CONFIG_MAX_VIEWS

inline bool
validate_view_z_index(int16_t z_index)
{
    int16_t min_z_index, max_z_index;
    min_z_index = (KAACORE_MAX_VIEWS) / -2;
    max_z_index = KAACORE_MAX_VIEWS + min_z_index - 1;
    return (min_z_index <= z_index) and (z_index <= max_z_index);
}

enum class ClearFlag {
    none = BGFX_CLEAR_NONE,
    color = BGFX_CLEAR_COLOR,
    depth = BGFX_CLEAR_DEPTH,
    stencil = BGFX_CLEAR_STENCIL
};

uint16_t
operator|(ClearFlag left, ClearFlag right);
uint16_t
operator|(ClearFlag left, uint16_t right);

class Renderer;
class ViewsManager;

class View {
  public:
    Camera camera;

    View& operator=(const View&) = delete;

    uint16_t index() const;
    void view_rect(const glm::ivec2& origin, const glm::uvec2& dimensions);
    std::pair<glm::ivec2, glm::uvec2> view_rect() const;
    void clear(
        const glm::dvec4& color = {0, 0, 0, 1},
        const uint16_t flags = ClearFlag::color | ClearFlag::depth);

  private:
    uint16_t _index;
    bool _is_dirty;
    bool _requires_clean;

    uint16_t _clear_flags;
    glm::dvec4 _view_rect;
    glm::uvec2 _dimensions;
    glm::dvec4 _clear_color;
    glm::ivec2 _origin = {0, 0};
    glm::fmat4 _projection_matrix;

    View();

    void _refresh();

    friend class Renderer;
    friend class ViewsManager;
};

class Scene;

class ViewsManager {
  public:
    ViewsManager();
    View& operator[](const int16_t z_index);
    View* begin();
    View* end();

    size_t size();

  private:
    View _views[KAACORE_MAX_VIEWS];

    void _mark_dirty();

    friend class Scene;
};

} // namespace kaacore
