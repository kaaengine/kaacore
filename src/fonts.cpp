#include <vector>

#include "kaacore/texture_loader.h"

#include "kaacore/fonts.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC

#include "stb_rect_pack.h"
#include "stb_truetype.h"


namespace kaacore {

std::pair<bimg::ImageContainer*, BakedFontData>
bake_font_texture(const RawFile& font_file)
{
    std::vector<uint8_t> pixels;
    pixels.reserve(font_baker_texture_size * font_baker_texture_size);
    stbtt_pack_context pack_ctx;
    BakedFontData baked_font_data;
    baked_font_data.resize(font_baker_glyphs_count);

    stbtt_PackBegin(&pack_ctx, pixels.data(),
                    font_baker_texture_size, font_baker_texture_size,
                    0, 1, nullptr);
    stbtt_PackFontRange(&pack_ctx, font_file.content.data(), 0,
                       font_baker_pixel_height, font_baker_first_glyph,
                       font_baker_glyphs_count, baked_font_data.data());
    stbtt_PackEnd(&pack_ctx);

    bimg::ImageContainer* baked_font_image = load_raw_image(
        bimg::TextureFormat::Enum::R8,
        font_baker_texture_size, font_baker_texture_size, pixels
    );

    return std::make_pair(baked_font_image, baked_font_data);
}


FontRenderGlyph::FontRenderGlyph(uint32_t character, stbtt_packedchar glyph_data,
                                 double scale_factor)
    : character(character), position(0., 0.)
{
    this->offset = glm::dvec2(glyph_data.xoff, glyph_data.yoff) * scale_factor;
    this->size = glm::dvec2(glyph_data.xoff2 - glyph_data.xoff,
                            glyph_data.yoff2 - glyph_data.yoff) * scale_factor;
    this->advance = glyph_data.xadvance * scale_factor;

    this->texture_uv0 = \
        glm::dvec2(glyph_data.x0, glyph_data.y0) * font_baker_inverted_texture_size;
    this->texture_uv1 = \
        glm::dvec2(glyph_data.x1, glyph_data.y1) * font_baker_inverted_texture_size;
}

FontRenderGlyph::FontRenderGlyph(
    uint32_t character, stbtt_packedchar glyph_data,
    double scale_factor, const FontRenderGlyph& other_glyph
) : FontRenderGlyph(character, glyph_data, scale_factor)
{
    this->position.x = other_glyph.position.x + other_glyph.advance;
}

Shape FontRenderGlyph::make_shape(const std::vector<FontRenderGlyph>& render_glyphs)
{
    std::vector<StandardVertexData> vertices;
    std::vector<VertexIndex> indices;

    auto glyphs_count = render_glyphs.size();

    vertices.reserve(glyphs_count * 4);
    indices.reserve(glyphs_count * 6);

    for (const FontRenderGlyph& rg : render_glyphs) {
        auto vertices_count = vertices.size();

        vertices.push_back(
            // Left-top vertex
            StandardVertexData::XY_UV(
                rg.position.x + rg.offset.x,
                rg.position.y + rg.offset.y,
                rg.texture_uv0.x, rg.texture_uv0.y
            )
        );
        vertices.push_back(
            // Right-top vertex
            StandardVertexData::XY_UV(
                rg.position.x + rg.offset.x + rg.size.x,
                rg.position.y + rg.offset.y,
                rg.texture_uv1.x, rg.texture_uv0.y
            )
        );
        vertices.push_back(
            // Left-bottom vertex
            StandardVertexData::XY_UV(
                rg.position.x + rg.offset.x,
                rg.position.y + rg.offset.y + rg.size.y,
                rg.texture_uv0.x, rg.texture_uv1.y
            )
        );
        vertices.push_back(
            // Right-bottom vertex
            StandardVertexData::XY_UV(
                rg.position.x + rg.offset.x + rg.size.x,
                rg.position.y + rg.offset.y + rg.size.y,
                rg.texture_uv1.x, rg.texture_uv1.y
            )
        );

        indices.push_back(vertices_count + 0);
        indices.push_back(vertices_count + 2);
        indices.push_back(vertices_count + 1);
        indices.push_back(vertices_count + 1);
        indices.push_back(vertices_count + 2);
        indices.push_back(vertices_count + 3);
    }

    return Shape::Freeform(indices, vertices);
}


Font::Font(const Resource<Image> baked_texture, const BakedFontData baked_font)
    : baked_texture(baked_texture), baked_font(baked_font)
{
}

Font Font::load(const std::string& font_filepath)
{
    RawFile file(font_filepath);
    bimg::ImageContainer* baked_font_image;
    BakedFontData baked_font_data;

    std::tie(baked_font_image, baked_font_data) = bake_font_texture(file);
    bgfx::TextureHandle texture = make_texture(baked_font_image);

    return Font{Image::load(texture, baked_font_image), baked_font_data};
}

std::vector<FontRenderGlyph> Font::generate_render_glyphs(
    const std::string& text, const double scale_factor
)
{
    std::vector<FontRenderGlyph> render_glyphs;

    for (const auto ch : text) {
        auto ch_value = static_cast<uint32_t>(ch) - font_baker_first_glyph;
        auto glyph_data = this->baked_font.at(ch_value);

        if (not render_glyphs.empty()) {
            render_glyphs.emplace_back(
                ch, glyph_data, scale_factor, render_glyphs.back()
            );
        } else {
            render_glyphs.emplace_back(
                ch, glyph_data, scale_factor
            );
        }
    }

    return render_glyphs;
}

} // namespace kaacore
