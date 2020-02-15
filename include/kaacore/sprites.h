#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "kaacore/images.h"
#include "kaacore/resources.h"

namespace kaacore {

struct Sprite {
    ResourceReference<Image> texture;
    // we assume that image contains it's width and height

    // origin points and dimensions
    glm::dvec2 origin;
    glm::dvec2 dimensions;

    Sprite();
    Sprite(ResourceReference<Image> texture);

    static Sprite load(const char* path, uint64_t flags = BGFX_SAMPLER_NONE);

    inline bool has_texture() const { return bool(this->texture); }
    inline operator bool() const { return this->has_texture(); }

    Sprite crop(glm::dvec2 new_origin, glm::dvec2 new_dimensions) const;
    Sprite crop(glm::dvec2 new_origin) const;
    std::pair<glm::dvec2, glm::dvec2> get_display_rect() const;
    glm::dvec2 get_size() const;
};

std::vector<Sprite>
split_spritesheet(
    const Sprite& spritesheet, const glm::dvec2 frame_dimensions,
    const size_t frames_offset = 0, const size_t frames_count = 0,
    const glm::dvec2 frame_padding = {0., 0.});

} // namespace kaacore
