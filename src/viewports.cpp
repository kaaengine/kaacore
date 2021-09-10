#include <algorithm>

#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/viewports.h"

namespace kaacore {

std::unordered_set<int16_t>
_translate_from_z_index(const std::unordered_set<int16_t>& indices)
{
    std::unordered_set<int16_t> result(indices.size());
    std::transform(
        indices.begin(), indices.end(), std::inserter(result, result.begin()),
        [](int16_t index) -> int16_t {
            KAACORE_CHECK(validate_z_index(index), "Invalid z index.");
            return index - viewports_min_z_index;
        });
    return result;
}

Viewport::Viewport()
    : _dimensions(get_engine()->virtual_resolution()), _is_dirty(true)
{}

int16_t
Viewport::z_index() const
{
    return this->_index - (KAACORE_MAX_VIEWPORTS / 2);
}

glm::ivec2
Viewport::origin() const
{
    return this->_origin;
}

void
Viewport::origin(const glm::ivec2& origin)
{
    if (this->_origin == origin) {
        return;
    }

    this->_origin = origin;
    this->_is_dirty = true;
}

glm::uvec2
Viewport::dimensions() const
{
    return this->_dimensions;
}

void
Viewport::dimensions(const glm::uvec2& dimensions)
{
    if (this->_dimensions == dimensions) {
        return;
    }

    this->_dimensions = dimensions;
    this->_is_dirty = true;
}

glm::ivec4
Viewport::view_rect() const
{
    return {this->_origin.x, this->_origin.y, this->_dimensions.x,
            this->_dimensions.y};
}

void
Viewport::_refresh()
{
    if (this->camera._is_dirty) {
        this->camera._refresh();
    }

    auto engine = get_engine();
    auto renderer = engine->renderer.get();
    auto drawable_area = static_cast<glm::dvec2>(renderer->view_size);
    auto border_size = static_cast<glm::dvec2>(renderer->border_size);
    auto virtual_resoultion =
        static_cast<glm::dvec2>(engine->virtual_resolution());
    auto virtual_origin = static_cast<glm::dvec2>(this->_origin);
    auto virtual_dimensions = static_cast<glm::dvec2>(this->_dimensions);
    auto view_dimensions =
        virtual_dimensions / virtual_resoultion * drawable_area;
    auto view_origin = virtual_origin / virtual_resoultion * drawable_area;
    // start drawing from the edge of border
    auto clipped_view_origin = glm::max(view_origin, {0., 0.});
    // clip view when views_orgin < border_size
    auto clipped_view_dimensions =
        view_dimensions - clipped_view_origin + view_origin;
    // clip view when views_dimensions + views_orgin > drawable_area
    clipped_view_dimensions =
        glm::min(clipped_view_dimensions, drawable_area - clipped_view_origin);
    // since the view might be clipped, adjust projection so it's the same as it
    // would be without clipping
    auto projection_size_factor = clipped_view_dimensions / view_dimensions;
    glm::dvec2 projection_displacement = glm::min({0., 0.}, virtual_origin);
    projection_displacement += glm::max(
        {0., 0.}, virtual_origin + virtual_dimensions - virtual_resoultion);

    this->_view_rect = {clipped_view_origin.x + border_size.x,
                        clipped_view_origin.y + border_size.y,
                        clipped_view_dimensions.x, clipped_view_dimensions.y};

    this->_projection_matrix = glm::ortho(
        -virtual_dimensions.x * projection_size_factor.x / 2 -
            projection_displacement.x / 2,
        virtual_dimensions.x * projection_size_factor.x / 2 -
            projection_displacement.x / 2,
        virtual_dimensions.y * projection_size_factor.y / 2 -
            projection_displacement.y / 2,
        -virtual_dimensions.y * projection_size_factor.y / 2 -
            projection_displacement.y / 2);

    this->_is_dirty = false;
}

bool
Viewport::_refresh_required() const
{
    return this->_is_dirty or this->camera._is_dirty;
}

glm::fmat4
Viewport::_view_matrix() const
{
    return this->camera._calculated_view;
}

ViewportsManager::ViewportsManager()
{
    for (auto index = 0; index < this->size(); ++index) {
        this->_viewports[index]._index = index;
    }
}

Viewport& ViewportsManager::operator[](const int16_t z_index)
{
    KAACORE_CHECK(validate_z_index(z_index), "Invalid z_index.");
    auto index = z_index + (this->size() / 2);
    return this->_viewports[index];
}

Viewport*
ViewportsManager::get(const int16_t z_index)
{
    return &this->operator[](z_index);
}

Viewport*
ViewportsManager::begin()
{
    return this->_viewports;
}

Viewport*
ViewportsManager::end()
{
    return &this->_viewports[this->size()];
}

size_t
ViewportsManager::size()
{
    return KAACORE_MAX_VIEWPORTS;
}

void
ViewportsManager::_mark_dirty()
{
    for (auto& viewport : *this) {
        viewport._is_dirty = true;
    }
}

ViewportStateArray
ViewportsManager::take_snapshot()
{
    ViewportStateArray result;
    for (auto& viewport : *this) {
        if (viewport._refresh_required()) {
            viewport._refresh();
        }
        result[viewport._index] = {viewport._view_rect, viewport._view_matrix(),
                                   viewport._projection_matrix};
    }
    return result;
}

ViewportIndexSet::ViewportIndexSet(
    const std::unordered_set<int16_t>& indices_set)
    : IndexSet(_translate_from_z_index(indices_set))
{}

ViewportIndexSet::operator std::unordered_set<int16_t>() const
{
    std::unordered_set<int16_t> result;
    this->each_active_z_index(
        [&result](int16_t z_index) { result.insert(z_index); });
    return result;
}

ViewportIndexSet::operator std::vector<int16_t>() const
{
    std::vector<int16_t> result;
    this->each_active_z_index(
        [&result](int16_t z_index) { result.push_back(z_index); });
    return result;
}

} // namespace kaacore
