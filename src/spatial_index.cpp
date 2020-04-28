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
    auto* wrapper = reinterpret_cast<NodeSpatialWrapper*>(node_wrapper_obj);
    wrapper->refresh();
    KAACORE_ASSERT(wrapper->bounding_box);
    return convert_bounding_box(wrapper->bounding_box);
}

cpHashValue
_node_hash(Node* node)
{
    return reinterpret_cast<cpHashValue>(node);
}

NodeSpatialWrapper::NodeSpatialWrapper(Node* node) : node(node) {}

std::vector<glm::dvec2>
calculate_shape_bounding_points(const Shape& shape)
{
    KAACORE_ASSERT(shape.type != ShapeType::none);
    std::vector<glm::dvec2> bounding_points;
    if (shape.type == ShapeType::polygon) {
        for (auto& pt : shape.points) {
            bounding_points = shape.points;
        }
    } else if (shape.type == ShapeType::circle) {
        // for circle we calculate N discrete points on circle's edge,
        // points are equally spaced.
        auto center = shape.points[0];
        for (int i = 0; i < circle_shape_generated_points_count; i++) {
            double t = (2 * M_PI / circle_shape_generated_points_count) * i;
            bounding_points.push_back(glm::dvec2(
                center.x + shape.radius * std::sin(t),
                center.y + shape.radius * std::cos(t)));
        }
    } else {
        // for other shape's we use original shape's bbox as bounding points
        if (shape.vertices_bbox) {
            bounding_points.push_back(glm::dvec2(
                shape.vertices_bbox.min_x, shape.vertices_bbox.min_y));
            bounding_points.push_back(glm::dvec2(
                shape.vertices_bbox.min_x, shape.vertices_bbox.max_y));
            bounding_points.push_back(glm::dvec2(
                shape.vertices_bbox.max_x, shape.vertices_bbox.max_y));
            bounding_points.push_back(glm::dvec2(
                shape.vertices_bbox.max_x, shape.vertices_bbox.min_y));
        } else {
            // or if bbox is NaN, use {0, 0} so it will
            // be transformed by node's transformation
            bounding_points.push_back({0., 0.});
        }
    }

    return bounding_points;
}

void
NodeSpatialWrapper::refresh()
{
    if (node->_spatial_data.is_dirty) {
        log<LogLevel::debug>(
            "Trigerred refresh of NodeSpatialWrapper of node: %p", this->node);
        BoundingBoxBuilder<double> bbox_builder;
        const auto node_transformation = node->absolute_transformation();
        const auto shape = node->_shape;
        if (shape) {
            const auto shape_transformation =
                Transformation::translate(calculate_realignment_vector(
                    node->_origin_alignment, shape.vertices_bbox)) |
                node_transformation;
            const auto new_shape_hash = std::hash<Shape>{}(shape);
            if (this->shape_hash != new_shape_hash) {
                this->bounding_points =
                    std::move(calculate_shape_bounding_points(shape));
                this->shape_hash = new_shape_hash;
            }
            this->bounding_points_transformed = this->bounding_points;
            for (auto& pt : this->bounding_points_transformed) {
                pt |= shape_transformation;
                bbox_builder.add_point(pt);
            }
        } else {
            this->bounding_points.clear();
            this->bounding_points_transformed.clear();
            bbox_builder.add_point(node->_position | node_transformation);
        }
        this->bounding_box = bbox_builder.bounding_box;
        log<LogLevel::debug>(
            " -> Resulting bbox x:(%lg, %lg) y:(%lg, %lg)",
            this->bounding_box.min_x, this->bounding_box.max_x,
            this->bounding_box.min_y, this->bounding_box.max_y);

        node->_spatial_data.is_dirty = false;
    }
}

bool
NodeSpatialWrapper::contains_point(const glm::dvec2 point) const
{
    return check_point_in_polygon(this->bounding_points_transformed, point);
}

SpatialIndex::SpatialIndex()
{
    this->_cp_index = cpBBTreeNew(_node_wrapper_bbfunc, nullptr);
}

void
_destroy_wrapper(void* obj, void* data)
{
    auto wrapper = reinterpret_cast<NodeSpatialWrapper*>(obj);
    delete wrapper;
}

SpatialIndex::~SpatialIndex()
{
    cpSpatialIndexEach(this->_cp_index, _destroy_wrapper, nullptr);
    cpSpatialIndexFree(this->_cp_index);
}

void
SpatialIndex::start_tracking(Node* node)
{
    KAACORE_ASSERT(node->_spatial_data.wrapper == nullptr);
    NodeSpatialWrapper* wrapper = new NodeSpatialWrapper(node);
    node->_spatial_data.wrapper = wrapper;
    cpSpatialIndexInsert(this->_cp_index, wrapper, _node_hash(node));
}

void
SpatialIndex::stop_tracking(Node* node)
{
    KAACORE_ASSERT(node->_spatial_data.wrapper != nullptr);
    cpSpatialIndexRemove(
        this->_cp_index, node->_spatial_data.wrapper, _node_hash(node));
    delete node->_spatial_data.wrapper;
    node->_spatial_data.wrapper = nullptr;
}

void
SpatialIndex::refresh_single(Node* node)
{
    KAACORE_ASSERT(node->_spatial_data.wrapper != nullptr);
    cpSpatialIndexReindexObject(
        this->_cp_index, node->_spatial_data.wrapper, _node_hash(node));
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
            results.push_back(wrapper->node);
        }
    }

    return results;
}

std::vector<NodePtr>
SpatialIndex::query_point(const glm::dvec2 point, bool include_shapeless)
{
    auto wrapper_results =
        this->_query_wrappers(BoundingBox{point.x, point.y, point.x, point.y});
    std::vector<NodePtr> results;
    for (auto wrapper : wrapper_results) {
        if ((include_shapeless or
             wrapper->bounding_points_transformed.size() > 1) and
            wrapper->contains_point(point)) {
            results.push_back(wrapper->node);
        }
    }

    return results;
}

cpCollisionID
_cp_spatial_index_query(
    void* obj, void* subtree_obj, cpCollisionID cid, void* data)
{
    auto wrapper_results =
        reinterpret_cast<std::vector<NodeSpatialWrapper*>*>(obj);
    auto wrapper = reinterpret_cast<NodeSpatialWrapper*>(subtree_obj);
    wrapper_results->push_back(wrapper);
    return cid;
}

std::vector<NodeSpatialWrapper*>
SpatialIndex::_query_wrappers(const BoundingBox<double>& bbox)
{
    std::vector<NodeSpatialWrapper*> wrapper_results;
    auto cp_bbox = convert_bounding_box(bbox);
    cpSpatialIndexQuery(
        this->_cp_index, reinterpret_cast<void*>(&wrapper_results), cp_bbox,
        _cp_spatial_index_query, nullptr);

    return wrapper_results;
}

} // namespace kaacore
