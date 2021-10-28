#pragma once

#include <cstdint>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <vector>

#include <bgfx/bgfx.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#undef GLM_ENABLE_EXPERIMENTAL

#include "kaacore/materials.h"
#include "kaacore/render_passes.h"
#include "kaacore/resources.h"
#include "kaacore/textures.h"
#include "kaacore/utils.h"
#include "kaacore/vertex_layout.h"
#include "kaacore/viewports.h"

namespace kaacore {

typedef size_t DrawUnitId;

struct DrawBucketKey {
    RenderPassIndexSet render_passes;
    ViewportIndexSet viewports;
    int16_t z_index;
    uint8_t root_distance;
    Texture* texture;
    Material* material;
    uint64_t state_flags;
    uint32_t stencil_flags;

    inline bool operator==(const DrawBucketKey& other) const
    {
        return (
            this->render_passes == other.render_passes and
            this->viewports == other.viewports and
            this->z_index == other.z_index and
            this->root_distance == other.root_distance and
            this->texture == other.texture and
            this->material == other.material and
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
                   this->render_passes, this->viewports, this->z_index,
                   this->root_distance, this->texture, this->material,
                   this->state_flags, this->stencil_flags) <
               std::tie(
                   other.render_passes, this->viewports, other.z_index,
                   other.root_distance, other.texture, other.material,
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

struct DrawUnitModificationPack {
    DrawUnitModificationPack(
        std::optional<DrawUnitModification> upsert_mod_,
        std::optional<DrawUnitModification> remove_mod_);

    explicit operator bool() const;
    std::pair<
        std::optional<DrawUnitModification>,
        std::optional<DrawUnitModification>>
    unpack();
    std::optional<DrawBucketKey> new_lookup_key() const;

    std::optional<DrawUnitModification> upsert_mod;
    std::optional<DrawUnitModification> remove_mod;
};

typedef std::pair<
    std::optional<DrawUnitModification>, std::optional<DrawUnitModification>>
    DrawUnitModificationPair;

class DrawBucket;

class GeometryStream {
  public:
    using DrawUnitIter = std::vector<DrawUnit>::const_iterator;
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

    inline bool empty() const { return this->_draw_units.empty(); }
    Range find_range() const;
    Range find_range(const DrawUnitIter start_pos) const;
    void copy_range(
        const Range& range, bgfx::TransientVertexBuffer& vertex_buffer,
        bgfx::TransientIndexBuffer& index_buffer) const;

  private:
    const std::vector<DrawUnit>& _draw_units;

    GeometryStream(const std::vector<DrawUnit>& draw_units);

    friend class DrawBucket;
};

typedef std::pair<
    std::optional<DrawUnitModification>, std::optional<DrawUnitModification>>
    DrawUnitModificationPair;

struct DrawBucket {
    GeometryStream geometry_stream() const;
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
            key.render_passes, key.viewports, key.z_index, key.root_distance,
            key.texture, key.material, key.state_flags, key.stencil_flags);
    }
};
}
