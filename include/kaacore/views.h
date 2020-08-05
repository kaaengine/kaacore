#pragma once

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include "kaacore/camera.h"
#include "kaacore/config.h"

namespace kaacore {

#define KAACORE_VIEWS_MIN_Z_INDEX (KAACORE_MAX_VIEWS / -2)
#define KAACORE_VIEWS_MAX_Z_INDEX ((KAACORE_MAX_VIEWS / 2) - 1)
#define KAACORE_VIEWS_DEFAULT_Z_INDEX 0

inline bool
validate_view_z_index(int16_t z_index)
{
    return (KAACORE_VIEWS_MIN_Z_INDEX <= z_index) and
           (z_index <= KAACORE_VIEWS_MAX_Z_INDEX);
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

class Renderer;
class ViewsManager;

class View {
  public:
    Camera camera;

    View& operator=(const View&) = delete;

    uint16_t internal_index() const;
    int16_t z_index() const;
    bool is_dirty() const;
    glm::ivec2 origin() const;
    void origin(const glm::ivec2& origin);
    glm::uvec2 dimensions() const;
    void dimensions(const glm::uvec2& dimensions);
    glm::dvec4 clear_color() const;
    void clear_color(const glm::dvec4& color);
    void reset_clear_color();

  private:
    uint16_t _index;
    bool _is_dirty;
    bool _requires_clean;

    glm::dvec4 _view_rect;
    glm::uvec2 _dimensions;
    glm::dvec4 _clear_color;
    glm::ivec2 _origin = {0, 0};
    glm::fmat4 _projection_matrix;
    uint16_t _clear_flags =
        ClearFlag::none | ClearFlag::depth | ClearFlag::color;

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
    View* get(const int16_t z_index);
    View* begin();
    View* end();

    size_t size();

  private:
    View _views[KAACORE_MAX_VIEWS];

    void _mark_dirty();

    friend class Scene;
};

} // namespace kaacore
