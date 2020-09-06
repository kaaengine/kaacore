#include "kaacore/camera.h"
#include "kaacore/engine.h"

namespace kaacore {

Camera::Camera()
{
    auto virtual_resolution = get_engine()->virtual_resolution();
    this->_position = {static_cast<double>(virtual_resolution.x) / 2,
                       static_cast<double>(virtual_resolution.y) / 2};
    this->refresh();
}

glm::dvec2
Camera::position() const
{
    return this->_position;
}

void
Camera::position(const glm::dvec2& position)
{
    if (this->_position != position) {
        this->_position = position;
        this->_is_dirty = true;
    }
}

double
Camera::rotation() const
{
    return this->_rotation;
}

void
Camera::rotation(const double rotation)
{
    if (this->_rotation != rotation) {
        this->_rotation = rotation;
        this->_is_dirty = true;
    }
}

glm::dvec2
Camera::scale() const
{
    return this->_scale;
}

void
Camera::scale(const glm::dvec2& scale)
{
    if (this->_scale != scale) {
        this->_scale = scale;
        this->_is_dirty = true;
    }
}

void
Camera::refresh()
{
    this->_calculated_view = glm::translate(
        glm::rotate(
            glm::scale(
                glm::fmat4(1.0),
                glm::fvec3(this->_scale.x, this->_scale.y, 1.)),
            static_cast<float>(this->_rotation), glm::fvec3(0., 0., 1.)),
        glm::fvec3(-this->_position.x, -this->_position.y, 0.));
    this->_is_dirty = false;
}

glm::dvec2
Camera::unproject_position(const glm::dvec2& pos)
{
    if (this->_is_dirty) {
        this->refresh();
    }

    auto virtual_resolution = get_engine()->virtual_resolution();
    // account for virtual_resolution / 2 since we want to get
    // top-left corner of camera 'window'
    glm::fvec4 pos4 = {pos.x - virtual_resolution.x / 2,
                       pos.y - virtual_resolution.y / 2, 0., 1.};
    pos4 = glm::inverse(this->_calculated_view) * pos4;
    return {pos4.x, pos4.y};
}

BoundingBox<double>
Camera::visible_area_bounding_box()
{
    auto virtual_resolution = get_engine()->virtual_resolution();
    return BoundingBox<double>::from_points(
        {this->unproject_position(glm::dvec2{-double(virtual_resolution.x) / 2,
                                             double(virtual_resolution.y) / 2}),
         this->unproject_position(glm::dvec2{double(virtual_resolution.x) / 2,
                                             double(virtual_resolution.y) / 2}),
         this->unproject_position(
             glm::dvec2{-double(virtual_resolution.x) / 2,
                        -double(virtual_resolution.y) / 2}),
         this->unproject_position(
             glm::dvec2{double(virtual_resolution.x) / 2,
                        -double(virtual_resolution.y) / 2})});
}

} // namespace kaacore
