#pragma once

#include <glm/glm.hpp>

#include "kaacore/resources.h"
#include "kaacore/texture_loader.h"


namespace kaacore {

struct Sprite {
    Resource<Image> texture;
    // we assume that image contains it's width and height

    // origin points and dimensions
    glm::dvec2 origin;
    glm::dvec2 dimensions;

    // animations stuff
    glm::dvec2 frame_dimensions = {0., 0.};

    uint16_t frame_offset = 0;
    uint16_t frame_count = 0;
    uint16_t frame_current = 0;
    bool animation_loop = false;

    // auto-timed animations
    uint16_t animation_frame_duration = 0;
    uint32_t animation_time_acc = 0;
    bool auto_animate = true;

    Sprite();
    Sprite(Resource<Image> texture);

    inline bool has_texture() const {return bool(this->texture);}
    inline operator bool() const {return this->has_texture();}

    Sprite crop(glm::dvec2 new_origin, glm::dvec2 new_dimensions) const;
    std::pair<glm::dvec2, glm::dvec2> get_display_rect() const;
    void animation_step(uint16_t step_size);
    void animation_time_step(uint16_t time_step_size);
};

} // namespace kaacore
