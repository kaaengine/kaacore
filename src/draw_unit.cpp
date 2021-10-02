#include <algorithm>
#include <limits>

#include "kaacore/log.h"

#include "kaacore/draw_unit.h"

namespace kaacore {

constexpr size_t range_max_vertices_count =
    std::numeric_limits<uint16_t>::max();
constexpr size_t range_max_indices_count = std::numeric_limits<uint32_t>::max();

DrawUnitModificationPack::DrawUnitModificationPack(
    std::optional<DrawUnitModification> upsert_mod_,
    std::optional<DrawUnitModification> remove_mod_)
    : upsert_mod(upsert_mod_), remove_mod(remove_mod_)
{
    if (upsert_mod_.has_value()) {
        KAACORE_ASSERT(
            upsert_mod_->type != DrawUnitModification::Type::update or
                not remove_mod_.has_value(),
            "`update` modification type cannot be combined with `remove` type");
    }
}

DrawUnitModificationPack::operator bool() const
{
    return this->upsert_mod.has_value() or this->remove_mod.has_value();
}

std::pair<
    std::optional<DrawUnitModification>, std::optional<DrawUnitModification>>
DrawUnitModificationPack::unpack()
{
    return {this->upsert_mod, this->remove_mod};
}

std::optional<DrawBucketKey>
DrawUnitModificationPack::new_lookup_key() const
{
    KAACORE_ASSERT(
        *this, "Can't get lookup key on empty `DrawUnitModificationPack`");
    if (this->upsert_mod) {
        return this->upsert_mod->lookup_key;
    }
    return std::nullopt;
}

DrawBucket::Range
DrawBucket::find_range() const
{
    return this->find_range(this->draw_units.cbegin());
}

DrawBucket::Range
DrawBucket::find_range(const DrawBucket::DrawUnitIter start_pos) const
{
    DrawBucket::Range range;
    range.begin = start_pos;
    range.vertices_count = 0;
    range.indices_count = 0;

    if (start_pos == this->draw_units.end()) {
        range.end = this->draw_units.end();
        return range;
    }
    DrawBucket::DrawUnitIter it;
    for (it = start_pos; it < this->draw_units.end(); it++) {
        const auto& unit = *it;
        // check buffer limits
        if (range.vertices_count + unit.details.vertices.size() >
                range_max_vertices_count or
            range.indices_count + unit.details.indices.size() >
                range_max_vertices_count) {
            break;
        }
        range.vertices_count += unit.details.vertices.size();
        range.indices_count += unit.details.indices.size();
    }
    range.end = it;

    return range;
}

void
DrawBucket::copy_range_details_to_transient_buffers(
    const DrawBucket::Range& range, bgfx::TransientVertexBuffer& vertex_buffer,
    bgfx::TransientIndexBuffer& index_buffer) const
{
    KAACORE_LOG_TRACE(
        "Loading {} ({} bytes) vertices / {} ({} bytes) indices to transient "
        "buffers",
        range.vertices_count, range.vertices_count * sizeof(StandardVertexData),
        range.indices_count, range.indices_count * sizeof(VertexIndex));
    KAACORE_LOG_TRACE(
        "BGFX vertices / indices buffer size: {} / {}", vertex_buffer.size,
        index_buffer.size);
    uint8_t* vertex_writer_pos = vertex_buffer.data;
    uint8_t* index_writer_pos = index_buffer.data;
    size_t indices_offset = 0;
    size_t vertices_count = 0;
    size_t indices_count = 0;
    for (DrawBucket::DrawUnitIter it = range.begin; it < range.end; it++) {
        const auto& unit = *it;

        vertices_count += unit.details.vertices.size();
        indices_count += unit.details.indices.size();

        KAACORE_ASSERT(
            vertices_count <= range.vertices_count,
            "Vertices count exceeded declared count ({} > {})", vertices_count,
            range.vertices_count);
        KAACORE_ASSERT(
            indices_count <= range.indices_count,
            "Indices count exceeded declared count ({} > {})", indices_count,
            range.indices_count);

        size_t vertex_data_size =
            unit.details.vertices.size() * sizeof(StandardVertexData);
        KAACORE_ASSERT(
            vertex_writer_pos + vertex_data_size <=
                vertex_buffer.data + vertex_buffer.size,
            "Write to transient vertex buffer would overflow");
        std::memcpy(
            vertex_writer_pos, unit.details.vertices.data(), vertex_data_size);
        vertex_writer_pos += vertex_data_size;

        size_t index_data_size =
            unit.details.indices.size() * sizeof(VertexIndex);
        KAACORE_ASSERT(
            index_writer_pos + index_data_size <=
                index_buffer.data + index_buffer.size,
            "Write to transient index buffer would overflow");
        std::memcpy(
            index_writer_pos, unit.details.indices.data(), index_data_size);
        for (VertexIndex* idx = (VertexIndex*)index_writer_pos;
             idx < (VertexIndex*)(index_writer_pos + index_data_size); idx++) {
            *idx += indices_offset;
        }
        index_writer_pos += index_data_size;
        indices_offset += unit.details.vertices.size();
    }

    KAACORE_ASSERT(
        vertex_writer_pos == vertex_buffer.data + vertex_buffer.size,
        "Vertex buffer wasn't fully filled (filled: {}, size: {})",
        vertex_writer_pos - vertex_buffer.data, vertex_buffer.size);
    KAACORE_ASSERT(
        index_writer_pos == index_buffer.data + index_buffer.size,
        "Index buffer wasn't fully filled (filled: {}, size: {})",
        index_writer_pos - index_buffer.data, index_buffer.size);
}

void
DrawBucket::consume_modifications(
    const std::vector<DrawUnitModification>::iterator src_begin,
    const std::vector<DrawUnitModification>::iterator src_end)
{
    thread_local std::vector<DrawUnit> tmp_buffer;
    tmp_buffer.clear();

    auto draw_unit_it = this->draw_units.begin();
    auto draw_unit_copy_it = draw_unit_it;

    for (auto mod_it = src_begin; mod_it != src_end; mod_it++) {
        KAACORE_ASSERT(
            mod_it->lookup_key == src_begin->lookup_key,
            "DrawBucket ({}): DrawUnitModification has different lookup_key, "
            "position: {}",
            fmt::ptr(this), mod_it - src_begin);
        draw_unit_it =
            std::lower_bound(draw_unit_it, this->draw_units.end(), *mod_it);

        // copy all draw units up to found one
        if (draw_unit_it != draw_unit_copy_it) {
            KAACORE_LOG_TRACE(
                "DrawBucket ({}): copying {} non-modified draw units",
                fmt::ptr(this), draw_unit_it - draw_unit_copy_it);
            tmp_buffer.insert(
                tmp_buffer.end(), draw_unit_copy_it, draw_unit_it);
            draw_unit_copy_it = draw_unit_it;
        }

        switch (mod_it->type) {
            case DrawUnitModification::Type::insert:
                KAACORE_LOG_TRACE(
                    "DrawBucket ({}): Inserting new draw unit with id: {}",
                    fmt::ptr(this), mod_it->id);
                KAACORE_ASSERT(
                    mod_it->updated_vertices_indices,
                    "DrawBucket ({}): Invalid flag state for DrawUnit "
                    "insertion",
                    fmt::ptr(this));
                KAACORE_ASSERT(
                    draw_unit_it == this->draw_units.end() or
                        mod_it->id != draw_unit_it->id,
                    "DrawBucket ({}): DrawUnit ({}) - with given id already "
                    "exists in draw "
                    "bucket",
                    fmt::ptr(this), draw_unit_it->id);
                tmp_buffer.emplace_back(
                    mod_it->id, std::move(mod_it->state_update));
                break;
            case DrawUnitModification::Type::update:
                KAACORE_LOG_TRACE(
                    "DrawBucket ({}): updating draw unit with id: {}",
                    fmt::ptr(this), mod_it->id);
                KAACORE_ASSERT(
                    mod_it->id == draw_unit_it->id,
                    "DrawBucket ({}): DrawUnit ({}) - DrawUnitModification "
                    "({}) id mismatch",
                    fmt::ptr(this), draw_unit_it->id, mod_it->id);
                KAACORE_ASSERT(
                    mod_it->updated_vertices_indices,
                    "DrawBucket ({}): Invalid flag state for DrawUnit update",
                    fmt::ptr(this));
                tmp_buffer.emplace_back(
                    mod_it->id, std::move(mod_it->state_update));
                draw_unit_it++;
                draw_unit_copy_it++;
                break;
            case DrawUnitModification::Type::remove:
                KAACORE_LOG_TRACE(
                    "DrawBucket ({}): Removing draw unit with id: {}",
                    fmt::ptr(this), mod_it->id);
                KAACORE_ASSERT(
                    mod_it->id == draw_unit_it->id,
                    "DrawBucket ({}): DrawUnit ({}) - DrawUnitModification "
                    "({}) id mismatch",
                    fmt::ptr(this), draw_unit_it->id, mod_it->id);
                draw_unit_it++;
                draw_unit_copy_it++;
                break;
        }
    }

    // copy remaining draw units after modifications queue is exhausted
    if (draw_unit_it != this->draw_units.end()) {
        KAACORE_LOG_TRACE(
            "DrawBucket ({}): copying {} non-modified draw units (tail)",
            fmt::ptr(this), this->draw_units.end() - draw_unit_it);
        tmp_buffer.insert(
            tmp_buffer.end(), draw_unit_it, this->draw_units.end());
    }

    std::swap(tmp_buffer, this->draw_units);
    KAACORE_LOG_TRACE(
        "DrawBucket ({}): size after modifications: {}", fmt::ptr(this),
        this->draw_units.size());
}

} // namespace kaacore
