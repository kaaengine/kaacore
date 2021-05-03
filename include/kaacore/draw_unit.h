#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

#include "kaacore/images.h"
#include "kaacore/resources.h"
// #include "kaacore/shaders.h"
#include "kaacore/materials.h"
#include "kaacore/utils.h"
#include "kaacore/views.h"

namespace kaacore {

typedef uint16_t VertexIndex;
typedef size_t DrawUnitId;

struct StandardVertexData {
    glm::fvec3 xyz;
    glm::fvec2 uv;
    glm::fvec2 mn;
    glm::fvec4 rgba;

    StandardVertexData(
        float x = 0., float y = 0., float z = 0., float u = 0., float v = 0.,
        float m = 0., float n = 0., float r = 1., float g = 1., float b = 1.,
        float a = 1.)
        : xyz(x, y, z), uv(u, v), mn(m, n), rgba(r, g, b, a){};

    static inline StandardVertexData xy_uv(float x, float y, float u, float v)
    {
        return StandardVertexData(x, y, 0., u, v);
    }

    static inline StandardVertexData xy_uv_mn(
        float x, float y, float u, float v, float m, float n)
    {
        return StandardVertexData(x, y, 0., u, v, m, n);
    }

    inline bool operator==(const StandardVertexData& other) const
    {
        return (
            this->xyz == other.xyz and this->uv == other.uv and
            this->mn == other.mn and this->rgba == other.rgba);
    }
};

typedef std::pair<std::vector<StandardVertexData>, std::vector<VertexIndex>>
    VerticesIndicesVectorPair;

struct DrawBucketKey {
    ViewIndexSet views;
    int16_t z_index;
    uint8_t root_distance;
    Image* texture_raw_ptr;
    Material* material_raw_ptr;
    uint64_t state_flags;
    uint32_t stencil_flags;

    inline bool operator==(const DrawBucketKey& other) const
    {
        return (
            this->views == other.views and this->z_index == other.z_index and
            this->root_distance == other.root_distance and
            this->texture_raw_ptr == other.texture_raw_ptr and
            this->material_raw_ptr == other.material_raw_ptr and
            this->state_flags == other.state_flags and
            this->stencil_flags == other.stencil_flags);
    }

    inline bool operator!=(const DrawBucketKey& other) const
    {
        return not(*this == other);
    }

    inline bool operator<(const DrawBucketKey& other) const
    {
        return std::tie(
                   this->views, this->z_index, this->root_distance,
                   this->texture_raw_ptr, this->material_raw_ptr,
                   this->state_flags, this->stencil_flags) <
               std::tie(
                   other.views, other.z_index, other.root_distance,
                   other.texture_raw_ptr, other.material_raw_ptr,
                   other.state_flags, other.stencil_flags);
    }
};

struct DrawUnitDetails {
    DrawUnitDetails() = default;
    DrawUnitDetails(
        std::vector<StandardVertexData>&& vertices,
        std::vector<VertexIndex>&& indices)
        : vertices(std::move(vertices)), indices(std::move(indices))
    {}

    std::vector<StandardVertexData> vertices;
    std::vector<VertexIndex> indices;
};

struct DrawUnitModification {
    enum struct Type : uint8_t {
        insert = 1,
        update = 2,
        remove = 3,
    };

    DrawUnitModification() = default;
    DrawUnitModification(
        const DrawUnitModification::Type type, const DrawBucketKey& lookup_key,
        const DrawUnitId id)
        : lookup_key(lookup_key), id(id), type(type)
    {}

    inline bool operator<(const DrawUnitModification& other) const
    {
        return std::tie(this->lookup_key, this->id, this->type) <
               std::tie(other.lookup_key, other.id, other.type);
    }

    DrawBucketKey lookup_key;
    DrawUnitId id;
    Type type;
    bool updated_vertices_indices;
    DrawUnitDetails state_update;
};

struct DrawUnit {
    DrawUnit(DrawUnitId id, DrawUnitDetails&& details)
        : id(id), details(std::move(details))
    {}

    inline bool operator<(const DrawUnit& other) const
    {
        return this->id < other.id;
    }

    inline bool operator<(const DrawUnitModification& mod) const
    {
        return this->id < mod.id;
    }

    DrawUnitId id;
    DrawUnitDetails details;
};

typedef std::pair<
    std::optional<DrawUnitModification>, std::optional<DrawUnitModification>>
    DrawUnitModificationPair;

struct DrawBucket {
    using DrawUnitIter = std::vector<DrawUnit>::const_iterator;
    using ModsIter = std::vector<DrawUnitModification>::const_iterator;

    struct Range {
        DrawUnitIter begin;
        DrawUnitIter end;
        size_t vertices_count;
        size_t indices_count;

        inline bool empty() const
        {
            return this->begin == this->end or this->vertices_count == 0;
        }
    };

    Range find_range() const;
    Range find_range(const DrawUnitIter start_pos) const;
    void copy_range_details_to_transient_buffers(
        const Range& range, bgfx::TransientVertexBuffer& vertex_buffer,
        bgfx::TransientIndexBuffer& index_buffer) const;

    void consume_modifications(
        const std::vector<DrawUnitModification>::iterator src_begin,
        const std::vector<DrawUnitModification>::iterator src_end);

    std::vector<DrawUnit> draw_units;
};

} // namespace kaacore

namespace std {
template<>
struct hash<kaacore::StandardVertexData> {
    size_t operator()(const kaacore::StandardVertexData& svd) const
    {
        return kaacore::hash_combined(svd.xyz, svd.uv, svd.mn, svd.rgba);
    }
};

template<>
struct hash<kaacore::DrawBucketKey> {
    size_t operator()(const kaacore::DrawBucketKey& key) const
    {
        return kaacore::hash_combined(
            key.views, key.z_index, key.root_distance, key.texture_raw_ptr,
            key.material_raw_ptr, key.state_flags, key.stencil_flags);
    }
};
}
