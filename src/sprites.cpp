#include <utility>

#include "kaacore/exceptions.h"
#include "kaacore/log.h"
#include "kaacore/sprites.h"

namespace kaacore {

Sprite::Sprite() : texture(), origin(0, 0), dimensions(0, 0) {}

Sprite::Sprite(ResourceReference<Image> texture)
    : texture(texture), origin(0, 0), dimensions(texture->get_dimensions())
{}

Sprite
Sprite::load(const char* path, uint64_t flags)
{
    return Sprite(Image::load(path, flags));
}

Sprite
Sprite::crop(glm::dvec2 new_origin, glm::dvec2 new_dimensions) const
{
    if (new_origin.x > this->dimensions.x) {
        log<LogLevel::warn>("Requested origin.x is greater than original");
    }
    if (new_origin.y > this->dimensions.y) {
        log<LogLevel::warn>("Requested origin.y is greater than original");
    }
    if (new_dimensions.x > this->dimensions.x - new_origin.x) {
        log<LogLevel::warn>("Requested dimensions.x is greater than available");
    }
    if (new_dimensions.y > this->dimensions.y - new_origin.y) {
        log<LogLevel::warn>("Requested dimensions.y is greater than available");
    }
    Sprite new_sprite(*this);
    new_sprite.origin = this->origin + new_origin;
    if (new_dimensions == glm::dvec2(0., 0.)) {
        new_sprite.dimensions = this->dimensions - new_origin;
    } else {
        new_sprite.dimensions = new_dimensions;
    }
    return new_sprite;
}

Sprite
Sprite::crop(glm::dvec2 new_origin) const
{
    return this->crop(new_origin, glm::dvec2(0., 0.));
}

std::pair<glm::dvec2, glm::dvec2>
Sprite::get_display_rect() const
{
    auto texture_dimensions = this->texture->get_dimensions();
    return std::make_pair(
        glm::dvec2(
            this->origin.x / texture_dimensions.x,
            this->origin.y / texture_dimensions.y),
        glm::dvec2(
            (this->origin.x + this->dimensions.x) / texture_dimensions.x,
            (this->origin.y + this->dimensions.y) / texture_dimensions.y));
}

glm::dvec2
Sprite::get_size() const
{
    return this->dimensions;
}

std::vector<Sprite>
split_spritesheet(
    const Sprite& spritesheet, const glm::dvec2 frame_dimensions,
    const size_t frames_offset, const size_t frames_count,
    const glm::dvec2 frame_padding)
{
    KAACORE_CHECK(spritesheet.has_texture());
    KAACORE_CHECK(frame_dimensions.x > 0 and frame_dimensions.y > 0);
    std::vector<Sprite> frames;

    glm::dvec2 spritesheet_dimensions = spritesheet.get_size();
    uint32_t columns_count =
        spritesheet_dimensions.x / (frame_dimensions.x + 2 * frame_padding.x);
    uint32_t rows_count =
        spritesheet_dimensions.y / (frame_dimensions.y + 2 * frame_padding.y);
    uint32_t max_frames_count = columns_count * rows_count;

    KAACORE_CHECK(frames_offset < max_frames_count);
    KAACORE_CHECK(frames_offset + frames_count < max_frames_count);

    uint32_t starting_col = frames_offset % columns_count;
    uint32_t starting_row = frames_offset / columns_count;
    uint32_t ending_col;
    uint32_t ending_row;

    if (frames_count > 0) {
        ending_col = (frames_offset + frames_count) % columns_count;
        ending_row = (frames_offset + frames_count) / columns_count;
        frames.reserve(frames_count);
    } else {
        ending_col = columns_count - 1;
        ending_row = rows_count - 1;
        frames.reserve(max_frames_count - frames_offset);
    }

    log<LogLevel::debug, LogCategory::misc>(
        "Starting grid spritesheet splitter, columns_count: %lu, rows_count: "
        "%lu, "
        "starting_pos: %lux%lu, ending_pos: %lux%lu.",
        columns_count, rows_count, starting_col, starting_row, ending_col,
        ending_row);

    size_t row = starting_row;
    size_t col = starting_col;
    while (true) {
        glm::dvec2 crop_point = {
            (frame_dimensions.x + 2 * frame_padding.x) * col + frame_padding.x,
            (frame_dimensions.y + 2 * frame_padding.y) * row + frame_padding.y};
        frames.push_back(
            std::move(spritesheet.crop(crop_point, frame_dimensions)));
        log<LogLevel::debug, LogCategory::misc>(
            "Cropped spritesheet frame %llu [%llux%llu] (%lf, %lf).",
            frames.size() - 1, col, row, crop_point.x, crop_point.y);

        if (row == ending_row and col == ending_col) {
            break;
        }

        col++;
        if (col >= columns_count) {
            col = 0;
            row++;
        }
    }

    return frames;
}

} // namespace kaacore
