#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include "kaacore/resources.h"
#include "kaacore/textures.h"
#include "kaacore/utils.h"

namespace kaacore {

struct Sprite {
    ResourceReference<Texture> texture;
    // we assume that image contains it's width and height

    // origin points and dimensions
    glm::dvec2 origin;
    glm::dvec2 dimensions;

    Sprite();
    Sprite(const ResourceReference<Texture>& texture);
    static Sprite load(const std::string& path);

    inline bool has_texture() const { return bool(this->texture); }
    inline operator bool() const { return this->has_texture(); }
    bool can_query() const;
    glm::dvec4 query_pixel(const glm::dvec2 position) const;

    bool operator==(const Sprite& other);

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

namespace std {
using kaacore::hash_combined;
using kaacore::Sprite;

template<>
struct hash<Sprite> {
    size_t operator()(const Sprite& sprite) const
    {
        return hash_combined(sprite.texture, sprite.origin, sprite.dimensions);
    }
};
}
