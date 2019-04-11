#pragma once

#include <utility>

#include "stb_truetype.h"

#include "kaacore/resources.h"
#include "kaacore/texture_loader.h"
#include "kaacore/files.h"


namespace kaacore {

const size_t font_baker_texture_size = 2048;
const size_t font_baker_pixel_height = 40;
const size_t font_baker_first_glyph = 32;
const size_t font_baker_glyphs_count = 96;


struct Font {
    Resource<Image> baked_texture;
    stbtt_packedchar* baked_glyphs;

    Font(const char* font_filepath);
    
    void _bake_font_texture(const RawFile& font_file);
};


} // namespace kaacore
