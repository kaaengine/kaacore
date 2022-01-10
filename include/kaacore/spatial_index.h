#pragma once

#include <vector>

#include <chipmunk/chipmunk.h>
#include <glm/glm.hpp>

#include "kaacore/geometry.h"
#include "kaacore/node_ptr.h"

namespace kaacore {

class Node;

struct NodeSpatialData {
    uint64_t index_uid;
    bool is_indexed = false;
    BoundingBox<double> bounding_box;
    std::vector<glm::dvec2> bounding_points_transformed;

    void refresh();
    bool contains_point(const glm::dvec2 point) const;
};

class SpatialIndex {
  public:
    SpatialIndex();
    ~SpatialIndex();

    void start_tracking(Node* node);
    void stop_tracking(Node* node);
    void update_single(Node* node);
    std::vector<NodePtr> query_bounding_box(
        const BoundingBox<double>& bbox, bool include_shapeless = true);
    std::vector<NodePtr> query_point(const glm::dvec2 point);

  private:
    std::vector<NodeSpatialData*> _query_wrappers(
        const BoundingBox<double>& bbox);
    void _add_to_cp_index(Node* node);
    void _update_cp_index(Node* node);
    void _remove_from_cp_index(Node* node);

    cpSpatialIndex* _cp_index;
    uint64_t _index_counter;
};

} // namespace kaacore
