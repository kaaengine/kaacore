#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "kaacore/geometry.h"
#include "kaacore/node_ptr.h"

#include <chipmunk/chipmunk.h>

namespace kaacore {

class Node;

struct NodeSpatialWrapper {
    NodeSpatialWrapper(Node* node);

    void refresh();
    bool contains_point(const glm::dvec2 point) const;

    Node* const node;
    BoundingBox<double> bounding_box;
    std::vector<glm::dvec2> bounding_points;
    std::vector<glm::dvec2> bounding_points_transformed;
    size_t shape_hash = 0;
};

class SpatialIndex {
  public:
    SpatialIndex();
    ~SpatialIndex();

    void start_tracking(Node* node);
    void stop_tracking(Node* node);

    void refresh_single(Node* node);
    void refresh_all();

    std::vector<NodePtr> query_bounding_box(
        const BoundingBox<double>& bbox, bool include_shapeless = false);
    std::vector<NodePtr> query_point(
        const glm::dvec2 point, bool include_shapeless = false);

  private:
    std::vector<NodeSpatialWrapper*> _query_wrappers(
        const BoundingBox<double>& bbox);

    cpSpatialIndex* _cp_index;
};

} // namespace kaacore
