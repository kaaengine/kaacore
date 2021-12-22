#include <algorithm>

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
            return index - min_viewport_z_index;
        });
    return result;
}

Viewport::Viewport() : _dimensions(get_engine()->virtual_resolution()) {}

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
Viewport::viewport_rect() const
{
    return {this->_origin.x, this->_origin.y, this->_dimensions.x,
            this->_dimensions.y};
}

void
Viewport::_reset()
{
    if (this->camera._is_dirty) {
        this->camera._reset();
    }

    if (not this->_is_dirty) {
        return;
    }

    auto engine = get_engine();
    auto renderer = engine->renderer.get();
    auto drawable_area = static_cast<glm::dvec2>(renderer->view_size);
    auto virtual_resoultion =
        static_cast<glm::dvec2>(engine->virtual_resolution());
    auto virtual_origin = static_cast<glm::dvec2>(this->_origin);
    auto virtual_dimensions = static_cast<glm::dvec2>(this->_dimensions);
    auto viewport_dimensions =
        virtual_dimensions / virtual_resoultion * drawable_area;
    auto viewport_origin = virtual_origin / virtual_resoultion * drawable_area;
    // start drawing from the edge of border
    auto clipped_viewport_origin = glm::max(viewport_origin, {0., 0.});
    // viewport_orgin < border_size
    auto clipped_viewport_dimensions =
        viewport_dimensions - clipped_viewport_origin + viewport_origin;
    // viewport_dimensions + viewport_orgin > drawable_area
    clipped_viewport_dimensions = glm::min(
        clipped_viewport_dimensions, drawable_area - clipped_viewport_origin);

    this->_view_rect = {clipped_viewport_origin, clipped_viewport_dimensions};
    this->_viewport_rect = {viewport_origin, viewport_dimensions};

    auto scale = virtual_resoultion / virtual_dimensions;
    auto target_resolution = virtual_resoultion * scale;
    auto target_origin = virtual_origin * scale;

    auto offset = virtual_resoultion - virtual_dimensions;
    float left = -(virtual_dimensions.x + offset.x) / 2.f - target_origin.x;
    float right = left + target_resolution.x;
    float top = -(virtual_dimensions.y + offset.y) / 2.f - target_origin.y;
    float bottom = top + target_resolution.y;

    // aspect ratio correction
    float correction;
    float viewport_ratio = virtual_dimensions.x / virtual_dimensions.y;
    float target_ratio = virtual_resoultion.x / virtual_resoultion.y;
    if (viewport_ratio >= target_ratio) {
        // wide viewport, use full height
        correction = viewport_ratio / target_ratio;
        left *= correction;
        right *= correction;
    } else {
        // tall viewport, use full width
        correction = target_ratio / viewport_ratio;
        top *= correction;
        bottom *= correction;
    }

    this->_projection_matrix = glm::ortho(left, right, bottom, top);
    this->_is_dirty = false;
}

bool
Viewport::_reset_required() const
{
    return this->_is_dirty or this->camera._is_dirty;
}

ViewportState
Viewport::_take_snapshot()
{
    return {this->_view_rect, this->_viewport_rect,
            this->camera._calculated_view, this->_projection_matrix};
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
ViewportsManager::_take_snapshot()
{
    ViewportStateArray result;
    for (auto& viewport : *this) {
        if (viewport._reset_required()) {
            viewport._reset();
        }
        result[viewport._index] = viewport._take_snapshot();
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
