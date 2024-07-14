#include <utility>

#include "kaacore/exceptions.h"
#include "kaacore/log.h"
#include "kaacore/sprites.h"

namespace kaacore {

Sprite::Sprite() : texture(), origin(0, 0), dimensions(0, 0) {}

Sprite::Sprite(const ResourceReference<Texture>& texture)
    : texture(texture), origin(0, 0),
      dimensions(texture.res_ptr->get_dimensions())
{}

Sprite
Sprite::load(const std::string& path)
{
    return Sprite(ImageTexture::load(path));
}

bool
Sprite::can_query() const
{
    if (this->has_texture()) {
        return this->texture->can_query();
    }
    return false;
}

glm::dvec4
Sprite::query_pixel(const glm::dvec2 position) const
{
    return this->texture->query_pixel(glm::round(origin + position));
}

bool
Sprite::operator==(const Sprite& other)
{
    return (
        this->texture == other.texture and this->origin == other.origin and
        this->dimensions == other.dimensions
    );
}

Sprite
Sprite::crop(glm::dvec2 new_origin, glm::dvec2 new_dimensions) const
{
    if (new_origin.x > this->dimensions.x) {
        KAACORE_LOG_WARN(
            "Requested origin.x ({}) is greater than original ({})",
            new_origin.x, this->dimensions.x
        );
    }
    if (new_origin.y > this->dimensions.y) {
        KAACORE_LOG_WARN(
            "Requested origin.y is greater than original", new_origin.y,
            this->dimensions.y
        );
    }
    if (new_dimensions.x > this->dimensions.x - new_origin.x) {
        KAACORE_LOG_WARN(
            "Requested dimensions.x is greater than available", new_origin.x,
            this->dimensions.x - new_origin.x
        );
    }
    if (new_dimensions.y > this->dimensions.y - new_origin.y) {
        KAACORE_LOG_WARN(
            "Requested dimensions.y is greater than available", new_origin.y,
            this->dimensions.y - new_origin.y
        );
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
            this->origin.y / texture_dimensions.y
        ),
        glm::dvec2(
            (this->origin.x + this->dimensions.x) / texture_dimensions.x,
            (this->origin.y + this->dimensions.y) / texture_dimensions.y
        )
    );
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
    const glm::dvec2 frame_padding
)
{
    KAACORE_CHECK(spritesheet.has_texture(), "Invalid sprite sheet.");
    KAACORE_CHECK(
        frame_dimensions.x > 0 and frame_dimensions.y > 0,
        "frame dimensions have to be greater than zero."
    );
    std::vector<Sprite> frames;

    glm::dvec2 spritesheet_dimensions = spritesheet.get_size();
    uint32_t columns_count =
        spritesheet_dimensions.x / (frame_dimensions.x + 2 * frame_padding.x);
    uint32_t rows_count =
        spritesheet_dimensions.y / (frame_dimensions.y + 2 * frame_padding.y);
    uint32_t max_frames_count = columns_count * rows_count;

    KAACORE_CHECK(
        frames_offset < max_frames_count, "Invalid frames_offset parameter."
    );
    KAACORE_CHECK(
        frames_offset + frames_count <= max_frames_count,
        "Invalid frames_offset parameter."
    );

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

    KAACORE_LOG_DEBUG(
        "Starting grid sprite sheet splitter, columns_count: {}, rows_count: "
        "{}, "
        "starting_pos: {}x{}, ending_pos: {}x{}.",
        columns_count, rows_count, starting_col, starting_row, ending_col,
        ending_row
    );

    size_t row = starting_row;
    size_t col = starting_col;
    while (true) {
        glm::dvec2 crop_point = {
            (frame_dimensions.x + 2 * frame_padding.x) * col + frame_padding.x,
            (frame_dimensions.y + 2 * frame_padding.y) * row + frame_padding.y
        };
        frames.push_back(
            std::move(spritesheet.crop(crop_point, frame_dimensions))
        );
        KAACORE_LOG_DEBUG(
            "Cropped spritesheet frame {} [{}x{}] ({}, {}).", frames.size() - 1,
            col, row, crop_point.x, crop_point.y
        );

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
