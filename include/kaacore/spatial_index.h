#pragma once

#include <vector>

#include <chipmunk/chipmunk.h>
#include <glm/glm.hpp>

#include "kaacore/geometry.h"
#include "kaacore/node_ptr.h"

namespace kaacore {

class Node;

struct NodeSpatialData {
    void refresh();
    bool contains_point(const glm::dvec2 point) const;

    bool is_dirty = false;
    BoundingBox<double> bounding_box;
    std::vector<glm::dvec2> bounding_points_transformed;
    uint64_t index_uid;
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
        const BoundingBox<double>& bbox, bool include_shapeless = true);
    std::vector<NodePtr> query_point(const glm::dvec2 point);

  private:
    std::vector<NodeSpatialData*> _query_wrappers(
        const BoundingBox<double>& bbox);

    cpSpatialIndex* _cp_index;
    uint64_t _index_counter;
};

} // namespace kaacore
