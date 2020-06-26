#pragma once

#include <unordered_set>
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
    bool is_indexed = false;
    bool is_phony_indexed = false;
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

    void update_single(Node* node);
    void refresh_all();

    std::vector<NodePtr> query_bounding_box(
        const BoundingBox<double>& bbox, bool include_shapeless = true);
    std::vector<NodePtr> query_bounding_box_for_drawing(
        const BoundingBox<double>& bbox);
    std::vector<NodePtr> query_point(const glm::dvec2 point);

  private:
    std::vector<NodeSpatialData*> _query_wrappers(
        const BoundingBox<double>& bbox);
    void _add_to_cp_index(Node* node);
    void _remove_from_cp_index(Node* node);
    void _add_to_phony_index(Node* node);
    void _remove_from_phony_index(Node* node);

    cpSpatialIndex* _cp_index;
    std::unordered_set<Node*> _phony_index;
    uint64_t _index_counter;
};

} // namespace kaacore
