#include <bgfx/bgfx.h>

#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/views.h"

namespace kaacore {

uint16_t
operator~(ClearFlag flag)
{
    return ~static_cast<uint16_t>(flag);
}

uint16_t
operator|(ClearFlag left, ClearFlag right)
{
    return static_cast<uint16_t>(left) | static_cast<uint16_t>(right);
}

uint16_t
operator|(ClearFlag left, uint16_t right)
{
    return static_cast<uint16_t>(left) | right;
}

uint16_t
operator|(uint16_t left, ClearFlag right)
{
    return left | static_cast<uint16_t>(right);
}

uint16_t
operator|=(uint16_t& left, ClearFlag right)
{
    return left = left | static_cast<uint16_t>(right);
}

ViewIndexSet::ViewIndexSet(std::unordered_set<int16_t> z_indexes_set)
{
    for (auto view_z_index : z_indexes_set) {
        KAACORE_ASSERT(
            validate_view_z_index(view_z_index), "Invalid view z-index");
        this->_views_bitset.set(
            view_z_index + views_z_index_to_internal_offset);
    }
}

ViewIndexSet::ViewIndexSet(std::bitset<KAACORE_MAX_VIEWS> _bitset)
    : _views_bitset(_bitset)
{}

std::bitset<KAACORE_MAX_VIEWS>::reference ViewIndexSet::operator[](size_t pos)
{
    return this->_views_bitset[pos + views_z_index_to_internal_offset];
}

bool
ViewIndexSet::operator==(const ViewIndexSet& other) const
{
    return this->_views_bitset == other._views_bitset;
}

bool
ViewIndexSet::operator<(const ViewIndexSet& other) const
{
    // make sure that all bits can be contained in `unsigned long`
    // during comparison
    static_assert(sizeof(unsigned long) * 8 >= KAACORE_MAX_VIEWS);
    return this->_views_bitset.to_ulong() < other._views_bitset.to_ulong();
}

ViewIndexSet
ViewIndexSet::operator|(const ViewIndexSet& other) const
{
    return ViewIndexSet{this->_views_bitset | other._views_bitset};
}

ViewIndexSet ViewIndexSet::operator&(const ViewIndexSet& other) const
{
    return ViewIndexSet{this->_views_bitset & other._views_bitset};
}

ViewIndexSet&
ViewIndexSet::operator|=(const ViewIndexSet& other)
{
    this->_views_bitset |= other._views_bitset;
    return *this;
}

ViewIndexSet&
ViewIndexSet::operator&=(const ViewIndexSet& other)
{
    this->_views_bitset &= other._views_bitset;
    return *this;
}

ViewIndexSet::operator std::unordered_set<int16_t>() const
{
    std::unordered_set<int16_t> active_views_set;
    this->each_active_z_index([&active_views_set](int16_t view_z_index) {
        active_views_set.insert(view_z_index);
    });
    return active_views_set;
}

ViewIndexSet::operator std::vector<int16_t>() const
{
    std::vector<int16_t> active_views_vector;
    this->each_active_z_index([&active_views_vector](int16_t view_z_index) {
        active_views_vector.push_back(view_z_index);
    });
    return active_views_vector;
}

View::View()
    : _dimensions(get_engine()->virtual_resolution()), _is_dirty(true),
      _requires_clean(false)
{}

uint16_t
View::internal_index() const
{
    return this->_index;
}

int16_t
View::z_index() const
{
    return this->_index - views_reserved_offset - (KAACORE_MAX_VIEWS / 2);
}

bool
View::is_dirty() const
{
    return this->_is_dirty or this->camera._is_dirty;
}

glm::ivec2
View::origin() const
{
    return this->_origin;
}

void
View::origin(const glm::ivec2& origin)
{
    if (this->_origin == origin) {
        return;
    }

    this->_origin = origin;
    this->_is_dirty = true;
}

glm::uvec2
View::dimensions() const
{
    return this->_dimensions;
}

void
View::dimensions(const glm::uvec2& dimensions)
{
    if (this->_dimensions == dimensions) {
        return;
    }

    this->_dimensions = dimensions;
    this->_is_dirty = true;
}

glm::dvec4
View::clear_color() const
{
    return this->_clear_color;
}

void
View::clear_color(const glm::dvec4& color)
{
    this->_clear_color = color;
    this->_clear_flags |= ClearFlag::color;
    this->_requires_clean = true;
}

void
View::reset_clear_color()
{
    this->_clear_flags &= ~ClearFlag::color;
    this->_requires_clean = true;
}

void
View::_refresh()
{
    if (this->camera._is_dirty) {
        this->camera.refresh();
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

ViewsManager::ViewsManager()
{
    for (uint16_t view_index = 0; view_index < this->size(); ++view_index) {
        this->_views[view_index]._index = view_index + views_reserved_offset;
    }
}

View& ViewsManager::operator[](const int16_t z_index)
{
    KAACORE_CHECK(validate_view_z_index(z_index), "Invalid view z_index.");
    auto index = z_index + (this->size() / 2);
    return this->_views[index];
}

View*
ViewsManager::get(const int16_t z_index)
{
    return &this->operator[](z_index);
}

View*
ViewsManager::begin()
{
    return this->_views;
}

View*
ViewsManager::end()
{
    return &this->_views[this->size()];
}

size_t
ViewsManager::size()
{
    return KAACORE_MAX_VIEWS;
}

void
ViewsManager::_mark_dirty()
{
    for (auto& view : *this) {
        view._is_dirty = true;
    }
}

} // namespace kaacore
