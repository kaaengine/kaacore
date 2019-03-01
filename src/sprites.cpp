#include <utility>

#include "kaacore/sprites.h"
#include "kaacore/log.h"


namespace kaacore {

Sprite::Sprite()
    : texture(), origin(0, 0), dimensions(0, 0) {}

Sprite::Sprite(Resource<Image> texture)
    : texture(texture), origin(0, 0), dimensions(texture->get_dimensions()) {}

Sprite Sprite::crop(glm::dvec2 new_origin, glm::dvec2 new_dimensions) const
{
    Sprite new_sprite(*this);
    new_sprite.origin = new_origin;
    new_sprite.dimensions = new_dimensions;
    return new_sprite;
}

std::pair<glm::dvec2, glm::dvec2> Sprite::get_display_rect() const
{
    auto texture_dimensions = this->texture->get_dimensions();
    double x, y, w, h;
    if (this->frame_dimensions != glm::dvec2(0., 0.)) {
        uint16_t frame_columns = this->dimensions.x / this->frame_dimensions.x;
        x = this->origin.x + (
            (this->frame_offset + this->frame_current) % frame_columns
        ) * this->frame_dimensions.x;
        y = this->origin.y + (
            (this->frame_offset + this->frame_current) / frame_columns
        ) * this->frame_dimensions.y;
        w = this->frame_dimensions.x;
        h = this->frame_dimensions.y;
    } else {
        x = this->origin.x;
        y = this->origin.y;
        w = this->dimensions.x;
        h = this->dimensions.y;
    }

    return std::make_pair(
        glm::dvec2(x / texture_dimensions.x, y / texture_dimensions.y),
        glm::dvec2((x + w) / texture_dimensions.x, (y + h) / texture_dimensions.y)
    );
}

void Sprite::animation_time_step(uint16_t time_step_size)
{
    if (this->frame_dimensions == glm::dvec2(0., 0.)
        or this->animation_frame_duration == 0) {
        return;
    }

    this->animation_time_acc += time_step_size;
    uint16_t step_size = this->animation_time_acc / this->animation_frame_duration;
    this->animation_time_acc = this->animation_time_acc % this->animation_frame_duration;

    this->animation_step(step_size);
}

void Sprite::animation_step(uint16_t step_size)
{
    if (this->frame_dimensions == glm::dvec2(0., 0.)) {
        return;
    }

    uint16_t effective_frame_count;
    if (this->frame_count > 0) {
        effective_frame_count = this->frame_count;
    } else {
        effective_frame_count = (
            uint16_t(this->dimensions.x / this->frame_dimensions.x) *
            uint16_t(this->dimensions.y / this->frame_dimensions.y) -
            this->frame_offset
        );
    }

    this->frame_current += step_size;
    if (this->frame_current >= effective_frame_count) {
        if (this->animation_loop) {
            this->frame_current = this->frame_current % effective_frame_count;
        } else {
            this->frame_current = effective_frame_count - 1;
        }
    }
}

} // namespace kaacore
