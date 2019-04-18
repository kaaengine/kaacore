#pragma once

#include <utility>
#include <vector>

#include "stb_truetype.h"

#include "kaacore/renderer.h"
#include "kaacore/shapes.h"
#include "kaacore/resources.h"
#include "kaacore/texture_loader.h"
#include "kaacore/files.h"


namespace kaacore {

typedef std::vector<stbtt_packedchar> BakedFontData;

const size_t font_baker_texture_size = 2048;
const double font_baker_inverted_texture_size = 1. / font_baker_texture_size;
const size_t font_baker_pixel_height = 60;
const size_t font_baker_first_glyph = 32;
const size_t font_baker_glyphs_count = 96;


struct FontRenderGlyph {
    uint32_t character;
    glm::dvec2 offset;
    glm::dvec2 size;
    glm::dvec2 position;
    glm::dvec2 texture_uv0;
    glm::dvec2 texture_uv1;
    double advance;
    
    FontRenderGlyph(uint32_t character, stbtt_packedchar glyph_data,
                    double scale_factor);
    FontRenderGlyph(uint32_t character, stbtt_packedchar glyph_data,
                    double scale_factor, const FontRenderGlyph& other_glyph);

    static Shape make_shape(const std::vector<FontRenderGlyph>& render_glyphs);
};


struct Font {
    Resource<Image> baked_texture;
    BakedFontData baked_font;

    Font(const Resource<Image> baked_texture, const BakedFontData baked_font);

    static Font load(const std::string& font_filepath);

    Shape generate_text_shape(const std::string& text, double size,
                              double indent, double max_width);

    std::vector<FontRenderGlyph> generate_render_glyphs(
        const std::string& text, const double scale_factor
    );
};


} // namespace kaacore
