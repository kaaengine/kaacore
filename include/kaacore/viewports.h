#pragma once

#include <array>

#include <glm/glm.hpp>

#include "kaacore/camera.h"
#include "kaacore/config.h"
#include "kaacore/indexset.h"

namespace kaacore {

constexpr auto default_viewport_z_index = 0;
constexpr auto min_viewport_z_index = (KAACORE_MAX_VIEWPORTS / -2);
constexpr auto max_viewport_z_index = ((KAACORE_MAX_VIEWPORTS / 2) - 1);

struct ViewportState;
using ViewportStateArray = std::array<ViewportState, KAACORE_MAX_VIEWPORTS>;

inline bool
validate_z_index(const int16_t z_index)
{
    return (min_viewport_z_index <= z_index) and
           (z_index <= max_viewport_z_index);
}

class Scene;
class ViewportsManager;

struct ViewportState {
    // rect clipped to drawable area - used for scissor test
    glm::fvec4 view_rect;
    // user defined rect - no cliping applied
    glm::fvec4 viewport_rect;
    glm::fmat4 view_matrix;
    glm::fmat4 projection_matrix;
};

class Viewport {
  public:
    Camera camera;

    Viewport& operator=(const Viewport&) = delete;

    int16_t z_index() const;
    glm::ivec2 origin() const;
    void origin(const glm::ivec2& origin);
    glm::uvec2 dimensions() const;
    void dimensions(const glm::uvec2& dimensions);
    glm::ivec4 viewport_rect() const;

  private:
    int16_t _index;
    bool _is_dirty = true;
    glm::uvec2 _dimensions;
    glm::ivec2 _origin = {0, 0};
    glm::fmat4 _projection_matrix;
    glm::dvec4 _view_rect;
    glm::dvec4 _viewport_rect;

    Viewport();

    void _reset();
    bool _reset_required() const;
    ViewportState _take_snapshot();

    friend class ViewportsManager;
};

class ViewportsManager {
  public:
    ViewportsManager();
    Viewport& operator[](const int16_t index);
    Viewport* get(const int16_t index);
    Viewport* begin();
    Viewport* end();
    size_t size();

  private:
    Viewport _viewports[KAACORE_MAX_VIEWPORTS];

    void _mark_dirty();
    ViewportStateArray _take_snapshot();

    friend Scene;
};

class ViewportIndexSet : public IndexSet<KAACORE_MAX_VIEWPORTS> {
  public:
    ViewportIndexSet() = default;
    ViewportIndexSet(const std::unordered_set<int16_t>& indices_set);
    operator std::unordered_set<int16_t>() const;
    operator std::vector<int16_t>() const;

    template<typename Func>
    void each_active_z_index(Func&& func) const
    {
        auto translate_index = [&func](int32_t internal_index) {
            func(internal_index + min_viewport_z_index);
        };
        this->each_active_index(translate_index);
    }

    friend struct std::hash<ViewportIndexSet>;
};

} // namespace kaacore

namespace std {
template<>
struct hash<kaacore::ViewportIndexSet> {
    size_t operator()(const kaacore::ViewportIndexSet& viewports) const
    {
        return std::hash<std::bitset<KAACORE_MAX_VIEWPORTS>>{}(
            viewports._bitset);
    }
};
}
