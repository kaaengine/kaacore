#pragma once

#include <string>

#include <glm/glm.hpp>
#include <bgfx/bgfx.h>

#include "kaacore/camera.h"
#include "../third_party/bgfx/bgfx/src/config.h"


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

class ViewsManager;

class View {
  public:
    Camera camera;

    View& operator=(const View&) = delete;

    bool is_dirty();
    void refresh();
    uint16_t index();
    void view_rect(const glm::ivec2& origin, const glm::uvec2& dimensions);
    std::pair<glm::ivec2, glm::uvec2> view_rect();
    void clear_color(const glm::dvec4& color);

  private:
    uint16_t _index;
    bool _is_dirty;
    glm::uvec2 _dimensions;
    glm::ivec2 _origin = {0, 0};
    uint32_t _clear_flags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;

    View();

    friend class ViewsManager;
};

class ViewsManager {
  public:
    ViewsManager();
    void mark_dirty();
    View& operator[](const int16_t z_index);

    size_t size();

  private:
    View _views[KAACORE_MAX_VIEWS];
};

} // namespace kaacore
