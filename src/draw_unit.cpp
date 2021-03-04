#include <algorithm>
#include <limits>

#include "kaacore/log.h"

#include "kaacore/draw_unit.h"

namespace kaacore {

constexpr size_t range_max_vertices_count =
    std::numeric_limits<uint16_t>::max();
constexpr size_t range_max_indices_count = std::numeric_limits<uint32_t>::max();

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
    // XXX expensive assertion
    KAACORE_ASSERT(
        std::is_sorted(src_begin, src_end), "Modifications are not sorted");

    auto draw_unit_it = this->draw_units.begin();
    for (auto mod_it = src_begin; mod_it != src_end; mod_it++) {
        KAACORE_ASSERT(
            mod_it->lookup_key == src_begin->lookup_key,
            "DrawUnitModification has different lookup_key, position: {}",
            mod_it - src_begin);
        draw_unit_it =
            std::lower_bound(draw_unit_it, this->draw_units.end(), *mod_it);

        switch (mod_it->type) {
            case DrawUnitModification::Type::insert:
                KAACORE_LOG_TRACE(
                    "Inserting new draw unit with id: {}", mod_it->id);
                KAACORE_ASSERT(
                    mod_it->updated_vertices_indices,
                    "Invalid flag state for DrawUnit insertion");
                KAACORE_ASSERT(
                    draw_unit_it == this->draw_units.end() or
                        mod_it->id != draw_unit_it->id,
                    "DrawUnit ({}) - with given id already exists in draw "
                    "bucket",
                    draw_unit_it->id);
                draw_unit_it = this->draw_units.emplace(
                    draw_unit_it, mod_it->id, std::move(mod_it->state_update));
                break;
            case DrawUnitModification::Type::update:
                KAACORE_LOG_TRACE("Updating draw unit with id: {}", mod_it->id);
                KAACORE_ASSERT(
                    mod_it->id == draw_unit_it->id,
                    "DrawUnit ({}) - DrawUnitModification ({}) id mismatch",
                    draw_unit_it->id, mod_it->id);
                KAACORE_ASSERT(
                    mod_it->updated_vertices_indices,
                    "Invalid flag state for DrawUnit update");
                draw_unit_it->details = std::move(mod_it->state_update);
                break;
            case DrawUnitModification::Type::remove:
                KAACORE_LOG_TRACE("Removing draw unit with id: {}", mod_it->id);
                KAACORE_ASSERT(
                    mod_it->id == draw_unit_it->id,
                    "DrawUnit ({}) - DrawUnitModification ({}) id mismatch",
                    draw_unit_it->id, mod_it->id);
                draw_unit_it = this->draw_units.erase(draw_unit_it);
                break;
        }
    }

    // XXX expensive assertion
    KAACORE_ASSERT(
        std::is_sorted(this->draw_units.begin(), this->draw_units.end()),
        "DrawBucket content is not sorted");
}

} // namespace kaacore
