#pragma once

#include <glm/glm.hpp>

#include "kaacore/geometry.h"

namespace kaacore {

class Viewport;
class Renderer;

class Camera {
  public:
    Camera();
    glm::dvec2 position() const;
    void position(const glm::dvec2& position);
    double rotation() const;
    void rotation(const double rotation);
    glm::dvec2 scale() const;
    void scale(const glm::dvec2& scale);
    glm::dvec2 unproject_position(const glm::dvec2& position);
    BoundingBox<double> visible_area_bounding_box();

  private:
    bool _is_dirty;
    glm::dvec2 _position;
    double _rotation = 0.;
    glm::dvec2 _scale = {1., 1.};
    glm::fmat4 _calculated_view;

    void _refresh();

    friend class Viewport;
};

} // namespace kaacore
