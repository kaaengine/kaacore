#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "kaacore/views.h"
#include "kaacore/log.h"
#include "kaacore/engine.h"
#include "kaacore/exceptions.h"

namespace kaacore {

uint16_t _to_view_index(int16_t z_index)
{
    KAACORE_CHECK(validate_view_z_index(z_index));
    return z_index + (KAACORE_MAX_VIEWS / 2);
}

View::View() : _dimensions(get_engine()->virtual_resolution()), _is_dirty(true)
{}

bool
View::is_dirty()
{
    return this->_is_dirty or this->camera._is_dirty;
}

void
View::refresh()
{
    if (this->camera._is_dirty) {
        this->camera.refresh();
    }

    auto engine = get_engine();
    auto renderer = engine->renderer.get();
    auto drawable_area = static_cast<glm::dvec2>(renderer->view_size);
    auto border_size = static_cast<glm::dvec2>(renderer->border_size);
    auto virtual_resoultion = static_cast<glm::dvec2>(engine->virtual_resolution());
    auto virtual_origin = static_cast<glm::dvec2>(this->_origin);
    auto virtual_dimensions = static_cast<glm::dvec2>(this->_dimensions);
    auto view_dimensions = virtual_dimensions / virtual_resoultion * drawable_area;
    auto view_origin = virtual_origin / virtual_resoultion * drawable_area;
    // start drawing from edge of border
    auto clipped_view_origin = glm::max(view_origin, {0., 0.});
    // clip view when views_orgin < border_size
    auto clipped_view_dimensions = view_dimensions - clipped_view_origin + view_origin;
    // clip view when vies_dimensions + views_orgin > drawable_area
    clipped_view_dimensions = glm::min(clipped_view_dimensions, drawable_area - clipped_view_origin);
    // since the view might be clipped, adjust projection so it's the same as it would be without clipping
    auto projection_size_factor = clipped_view_dimensions / view_dimensions;

    bgfx::setViewRect(
        this->_index,
        static_cast<uint16_t>(clipped_view_origin.x + border_size.x),
        static_cast<uint16_t>(clipped_view_origin.y + border_size.y),
        static_cast<uint16_t>(clipped_view_dimensions.x),
        static_cast<uint16_t>(clipped_view_dimensions.y)
    );

    // since the view might be clipped, adjust projection so it's the same as it would be without clipping
    glm::dvec2 projection_displacement = glm::min({0., 0.}, virtual_origin);
    projection_displacement += glm::max({0., 0.},  virtual_origin + virtual_dimensions - virtual_resoultion);

    glm::fmat4 projection_matrix = glm::ortho(
        -virtual_dimensions.x * projection_size_factor.x / 2 - projection_displacement.x / 2,
        virtual_dimensions.x * projection_size_factor.x / 2 - projection_displacement.x / 2,
        virtual_dimensions.y * projection_size_factor.y / 2 - projection_displacement.y / 2,
        -virtual_dimensions.y * projection_size_factor.y / 2 - projection_displacement.y / 2
    );

    bgfx::setViewTransform(
        this->_index,
        glm::value_ptr(this->camera._calculated_view),
        glm::value_ptr(projection_matrix)
    );

    this->_is_dirty = false;
}

uint16_t
View::index() {
    return this->_index;
}

void
View::view_rect(const glm::ivec2& origin, const glm::uvec2& dimensions)
{
    if (this->_origin == origin and this->_dimensions == dimensions) {
        return;
    }

    auto virtual_resilution = get_engine()->virtual_resolution();
    if (glm::any(glm::greaterThan(dimensions, virtual_resilution))) {
        log<LogLevel::warn, LogCategory::engine>(
            "View size greater than virtual resolution! %s > %s.",
            glm::to_string(dimensions).c_str(), glm::to_string(virtual_resilution).c_str()
        );
    }

    this->_origin = origin;
    this->_dimensions = dimensions;
    this->_is_dirty = true;
}

std::pair<glm::ivec2, glm::uvec2>
View::view_rect()
{
    return {this->_origin, this->_dimensions};
}

void
View::clear_color(const glm::dvec4& color)
{
    uint32_t r, g, b, a;
    a = static_cast<uint32_t>(color.a * 255.0 + 0.5);
    b = static_cast<uint32_t>(color.b * 255.0 + 0.5) << 8;
    g = static_cast<uint32_t>(color.g * 255.0 + 0.5) << 16;
    r = static_cast<uint32_t>(color.r * 255.0 + 0.5) << 24;
    auto clear_color_hex = a + b + g + r;
    bgfx::setViewClear(this->_index, this->_clear_flags, clear_color_hex);
    this->_is_dirty = true;
}

ViewsManager::ViewsManager()
{
    for(uint16_t view_index = 0; view_index < this->size(); ++view_index) {
        this->_views[view_index]._index = view_index;
    }
}

View&
ViewsManager::operator[](const int16_t z_index)
{
    return this->_views[_to_view_index(z_index)];
}

size_t
ViewsManager::size()
{
    return KAACORE_MAX_VIEWS;
}

void
ViewsManager::register_used_view(int16_t z_index)
{
    auto view_index = _to_view_index(z_index);
    this->_used_view_indices.insert(view_index);
}

void
ViewsManager::_touch()
{
    for (auto view_index : this->_used_view_indices) {
        bgfx::touch(view_index);
    }
}

void
ViewsManager::_mark_dirty()
{
    for (auto view_index : this->_used_view_indices) {
        this->_views[view_index]._is_dirty = true;
    }
}

} // namespace kaacore
