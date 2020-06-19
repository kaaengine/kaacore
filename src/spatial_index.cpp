#include <functional>
#include <vector>

#include <chipmunk/chipmunk.h>

#include "kaacore/exceptions.h"
#include "kaacore/log.h"
#include "kaacore/nodes.h"
#include "kaacore/shapes.h"

#include "kaacore/spatial_index.h"

namespace kaacore {

constexpr int circle_shape_generated_points_count = 24;

inline cpBB
convert_bounding_box(const BoundingBox<double>& bounding_box)
{
    return cpBBNew(
        bounding_box.min_x, bounding_box.min_y, bounding_box.max_x,
        bounding_box.max_y);
}

cpBB
_node_wrapper_bbfunc(void* node_wrapper_obj)
{
    auto* wrapper = reinterpret_cast<NodeSpatialData*>(node_wrapper_obj);
    wrapper->refresh();
    KAACORE_ASSERT(wrapper->bounding_box);
    return convert_bounding_box(wrapper->bounding_box);
}

inline constexpr Node*
container_node(const NodeSpatialData* spatial_data)
{
    return container_of(spatial_data, &Node::_spatial_data);
}

void
NodeSpatialData::refresh()
{
    if (this->is_dirty) {
        Node* node = container_node(this);
        log<LogLevel::debug>(
            "Trigerred refresh of NodeSpatialData of node: %p", node);
        const auto node_transformation = node->absolute_transformation();
        const auto shape = node->_shape;
        if (shape) {
            const auto shape_transformation =
                Transformation::translate(calculate_realignment_vector(
                    node->_origin_alignment, shape.vertices_bbox)) |
                node_transformation;
            this->bounding_points_transformed.resize(
                shape.bounding_points.size());
            std::transform(
                shape.bounding_points.begin(), shape.bounding_points.end(),
                this->bounding_points_transformed.begin(),
                [&shape_transformation](glm::dvec2 pt) -> glm::dvec2 {
                    return pt | shape_transformation;
                });
            this->bounding_box = BoundingBox<double>::from_points(
                this->bounding_points_transformed);
        } else {
            this->bounding_points_transformed.clear();
            this->bounding_box = BoundingBox<double>::single_point(
                node->_position | node_transformation);
        }
        log<LogLevel::debug>(
            " -> Resulting bbox x:(%lg, %lg) y:(%lg, %lg)",
            this->bounding_box.min_x, this->bounding_box.max_x,
            this->bounding_box.min_y, this->bounding_box.max_y);

        node->_spatial_data.is_dirty = false;
    }
}

bool
NodeSpatialData::contains_point(const glm::dvec2 point) const
{
    return check_point_in_polygon(this->bounding_points_transformed, point);
}

SpatialIndex::SpatialIndex() : _index_counter(0)
{
    this->_cp_index = cpBBTreeNew(_node_wrapper_bbfunc, nullptr);
}

SpatialIndex::~SpatialIndex()
{
    cpSpatialIndexFree(this->_cp_index);
}

void
SpatialIndex::start_tracking(Node* node)
{
    node->_spatial_data.is_dirty = true;
    node->_spatial_data.index_uid = ++this->_index_counter;
    cpSpatialIndexInsert(
        this->_cp_index, &node->_spatial_data, node->_spatial_data.index_uid);
}

void
SpatialIndex::stop_tracking(Node* node)
{
    cpSpatialIndexRemove(
        this->_cp_index, &node->_spatial_data, node->_spatial_data.index_uid);
}

void
SpatialIndex::refresh_single(Node* node)
{
    cpSpatialIndexReindexObject(
        this->_cp_index, &node->_spatial_data, node->_spatial_data.index_uid);
}

void
SpatialIndex::refresh_all()
{
    cpSpatialIndexReindex(this->_cp_index);
}

std::vector<NodePtr>
SpatialIndex::query_bounding_box(
    const BoundingBox<double>& bbox, bool include_shapeless)
{
    auto wrapper_results = this->_query_wrappers(bbox);
    std::vector<NodePtr> results;
    results.reserve(wrapper_results.size());
    for (auto wrapper : wrapper_results) {
        if (include_shapeless or
            wrapper->bounding_points_transformed.size() > 1) {
            results.push_back(container_node(wrapper));
        }
    }

    return results;
}

std::vector<NodePtr>
SpatialIndex::query_point(const glm::dvec2 point)
{
    auto wrapper_results =
        this->_query_wrappers(BoundingBox{point.x, point.y, point.x, point.y});
    std::vector<NodePtr> results;
    for (auto wrapper : wrapper_results) {
        if (wrapper->bounding_points_transformed.size() > 1 and
            wrapper->contains_point(point)) {
            results.push_back(container_node(wrapper));
        }
    }

    return results;
}

cpCollisionID
_cp_spatial_index_query(
    void* obj, void* subtree_obj, cpCollisionID cid, void* data)
{
    auto wrapper_results =
        reinterpret_cast<std::vector<NodeSpatialData*>*>(obj);
    auto wrapper = reinterpret_cast<NodeSpatialData*>(subtree_obj);
    wrapper_results->push_back(wrapper);
    return cid;
}

std::vector<NodeSpatialData*>
SpatialIndex::_query_wrappers(const BoundingBox<double>& bbox)
{
    std::vector<NodeSpatialData*> wrapper_results;
    auto cp_bbox = convert_bounding_box(bbox);
    cpSpatialIndexQuery(
        this->_cp_index, reinterpret_cast<void*>(&wrapper_results), cp_bbox,
        _cp_spatial_index_query, nullptr);

    return wrapper_results;
}

} // namespace kaacore
