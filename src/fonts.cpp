#include <cstring>
#include <memory>
#include <vector>

#include "stb_rect_pack.h"
#include "stb_truetype.h"

#include "kaacore/embedded_data.h"
#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/fonts.h"
#include "kaacore/images.h"
#include "kaacore/nodes.h"
#include "kaacore/utils.h"

#include "kaacore/fonts.h"

namespace kaacore {

ResourcesRegistry<std::string, FontData> _fonts_registry;

void
initialize_fonts()
{
    _fonts_registry.initialze();
    auto& default_font = get_default_font();
    if (not default_font._font_data->is_initialized) {
        default_font._font_data->_initialize();
    }
}

void
uninitialize_fonts()
{
    _fonts_registry.uninitialze();
    auto& default_font = get_default_font();
    if (default_font._font_data->is_initialized) {
        default_font._font_data->_uninitialize();
    }
}

std::pair<bimg::ImageContainer*, BakedFontData>
bake_font_texture(const uint8_t* font_file_content, const size_t size)
{
    if (memcmp(
            font_file_content, "\x00\x01\x00\x00\x00", size >= 5 ? 5 : size) !=
        0) {
        throw kaacore::exception(
            "Provided file is not TTF - magic number mismatched.");
    }
    std::vector<uint8_t> pixels_single;
    pixels_single.resize(font_baker_texture_size * font_baker_texture_size);
    stbtt_pack_context pack_ctx;
    BakedFontData baked_font_data;
    baked_font_data.resize(font_baker_glyphs_count);

    stbtt_PackBegin(
        &pack_ctx, pixels_single.data(), font_baker_texture_size,
        font_baker_texture_size, 0, 1, nullptr);
    stbtt_PackFontRange(
        &pack_ctx, font_file_content, 0, font_baker_pixel_height,
        font_baker_first_glyph, font_baker_glyphs_count,
        baked_font_data.data());
    stbtt_PackEnd(&pack_ctx);

    std::vector<uint8_t> pixels_rgba(pixels_single.size() * 4);
    for (size_t i = 0; i < pixels_single.size(); i++) {
        const uint8_t& px_src = pixels_single[i];
        uint8_t& px_dst_r = pixels_rgba[(i * 4) + 0];
        uint8_t& px_dst_g = pixels_rgba[(i * 4) + 1];
        uint8_t& px_dst_b = pixels_rgba[(i * 4) + 2];
        uint8_t& px_dst_a = pixels_rgba[(i * 4) + 3];
        px_dst_r = px_src;
        px_dst_g = px_src;
        px_dst_b = px_src;
        px_dst_a = px_src;
    }

    bimg::ImageContainer* baked_font_image = load_raw_image(
        bimg::TextureFormat::Enum::RGBA8, font_baker_texture_size,
        font_baker_texture_size, pixels_rgba);

    return std::make_pair(baked_font_image, baked_font_data);
}

std::pair<bimg::ImageContainer*, BakedFontData>
bake_font_texture(const RawFile& font_file)
{
    return bake_font_texture(
        font_file.content.data(), font_file.content.size());
}

FontRenderGlyph::FontRenderGlyph(
    uint32_t character, stbtt_packedchar glyph_data, double scale_factor)
    : character(character), position(0., 0.)
{
    this->offset = glm::dvec2(glyph_data.xoff, glyph_data.yoff) * scale_factor;
    this->size = glm::dvec2(
                     glyph_data.xoff2 - glyph_data.xoff,
                     glyph_data.yoff2 - glyph_data.yoff) *
                 scale_factor;
    this->advance = glyph_data.xadvance * scale_factor;

    this->texture_uv0 = glm::dvec2(glyph_data.x0, glyph_data.y0) *
                        font_baker_inverted_texture_size;
    this->texture_uv1 = glm::dvec2(glyph_data.x1, glyph_data.y1) *
                        font_baker_inverted_texture_size;
}

FontRenderGlyph::FontRenderGlyph(
    uint32_t character, stbtt_packedchar glyph_data, double scale_factor,
    const FontRenderGlyph& other_glyph)
    : FontRenderGlyph(character, glyph_data, scale_factor)
{
    this->position.x = other_glyph.position.x + other_glyph.advance;
}

void
FontRenderGlyph::arrange_glyphs(
    std::vector<FontRenderGlyph>& render_glyphs, const double indent,
    const double line_height, const double line_width)
{
    glm::dvec2 current_pos = {indent, 0.};
    std::vector<FontRenderGlyph>::iterator word_start = render_glyphs.begin();

    for (auto it = word_start; it != render_glyphs.end(); it++) {
        if (it->character == static_cast<uint32_t>(' ')) {
            word_start = it + 1;
            if (current_pos.x == 0.) {
                continue;
            }
        }
        if (it->character == static_cast<uint32_t>('\n')) {
            word_start = it + 1;
            current_pos.x = 0.;
            current_pos.y += line_height;
            continue;
        }
        it->position = current_pos;
        current_pos.x += it->advance;

        if (current_pos.x > line_width and
            ((it + 1) == render_glyphs.end() or
             (it + 1)->character == static_cast<uint32_t>(' '))) {
            current_pos.x = 0.;
            current_pos.y += line_height;

            for (auto word_it = word_start; word_it <= it; word_it++) {
                word_it->position = current_pos;
                current_pos.x += word_it->advance;
            }
            word_start = it + 1;
        }
    }
}

Shape
FontRenderGlyph::make_shape(const std::vector<FontRenderGlyph>& render_glyphs)
{
    if (render_glyphs.empty()) {
        return Shape{};
    }

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
                rg.position.x + rg.offset.x, rg.position.y + rg.offset.y,
                rg.texture_uv0.x, rg.texture_uv0.y));
        vertices.push_back(
            // Right-top vertex
            StandardVertexData::XY_UV(
                rg.position.x + rg.offset.x + rg.size.x,
                rg.position.y + rg.offset.y, rg.texture_uv1.x,
                rg.texture_uv0.y));
        vertices.push_back(
            // Left-bottom vertex
            StandardVertexData::XY_UV(
                rg.position.x + rg.offset.x,
                rg.position.y + rg.offset.y + rg.size.y, rg.texture_uv0.x,
                rg.texture_uv1.y));
        vertices.push_back(
            // Right-bottom vertex
            StandardVertexData::XY_UV(
                rg.position.x + rg.offset.x + rg.size.x,
                rg.position.y + rg.offset.y + rg.size.y, rg.texture_uv1.x,
                rg.texture_uv1.y));

        indices.push_back(vertices_count + 0);
        indices.push_back(vertices_count + 2);
        indices.push_back(vertices_count + 1);
        indices.push_back(vertices_count + 1);
        indices.push_back(vertices_count + 2);
        indices.push_back(vertices_count + 3);
    }

    return Shape::Freeform(indices, vertices);
}

FontData::FontData(const std::string& path) : path(path)
{
    RawFile file(this->path);
    auto [baked_font_image, baked_font_data] = bake_font_texture(file);
    this->baked_texture = Image::load(baked_font_image);
    this->baked_font = std::move(baked_font_data);

    if (is_engine_initialized()) {
        this->_initialize();
    }
}

FontData::FontData(
    const ResourceReference<Image> baked_texture,
    const BakedFontData baked_font)
{
    this->baked_texture = baked_texture;
    this->baked_font = std::move(baked_font);

    if (is_engine_initialized()) {
        this->_initialize();
    }
}

ResourceReference<FontData>
FontData::load(const std::string& path)
{
    std::shared_ptr<FontData> font_data;
    if ((font_data = _fonts_registry.get_resource(path))) {
        return font_data;
    }

    font_data = std::shared_ptr<FontData>(new FontData(path));
    _fonts_registry.register_resource(path, font_data);
    return font_data;
}

ResourceReference<FontData>
FontData::load_from_memory(const uint8_t* font_file_content, const size_t size)
{
    auto [baked_font_image, baked_font_data] =
        bake_font_texture(font_file_content, size);

    return std::shared_ptr<FontData>(
        new FontData(Image::load(baked_font_image), baked_font_data));
}

FontData::~FontData()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

std::vector<FontRenderGlyph>
FontData::generate_render_glyphs(
    const std::string& text, const double pixel_height)
{
    std::vector<FontRenderGlyph> render_glyphs;
    const double scale_factor = pixel_height / font_baker_pixel_height;

    for (const auto ch : text) {
        uint32_t ch_value;
        if (ch == '\n') {
            ch_value = static_cast<uint32_t>(' ') - font_baker_first_glyph;
        } else {
            ch_value = static_cast<uint32_t>(ch) - font_baker_first_glyph;
        }

        if (ch_value > font_baker_glyphs_count) {
            log<LogLevel::warn>("Unhadled font character: %llu", ch_value);
            ch_value = static_cast<uint32_t>('?') - font_baker_first_glyph;
        }

        auto glyph_data = this->baked_font.at(ch_value);

        if (not render_glyphs.empty()) {
            render_glyphs.emplace_back(
                ch, glyph_data, scale_factor, render_glyphs.back());
        } else {
            render_glyphs.emplace_back(ch, glyph_data, scale_factor);
        }
    }

    return render_glyphs;
}

void
FontData::_initialize()
{
    if (not this->baked_texture->is_initialized) {
        this->baked_texture->_initialize();
    }
    this->is_initialized = true;
}

void
FontData::_uninitialize()
{
    if (this->baked_texture->is_initialized) {
        this->baked_texture->_uninitialize();
    }
    this->is_initialized = false;
}

Font::Font() {}

Font::Font(const ResourceReference<FontData>& font_data) : _font_data(font_data)
{}

Font
Font::load(const std::string& font_filepath)
{
    return Font(FontData::load(font_filepath));
}

bool
Font::operator==(const Font& other)
{
    return this->_font_data == other._font_data;
}

Font&
get_default_font()
{
    static auto file_pair =
        get_embedded_file_content("embedded_resources/font_munro/munro.ttf");
    static Font default_font{
        FontData::load_from_memory(file_pair.first, file_pair.second)};
    return default_font;
}

inline constexpr Node*
container_node(const TextNode* text)
{
    return container_of(text, &Node::text);
}

TextNode::TextNode()
    : _content("TXT"), _font_size(28.), _line_width(INFINITY),
      _interline_spacing(1.), _first_line_indent(0.), _font(get_default_font())
{}

TextNode::~TextNode() {}

void
TextNode::_update_shape()
{
    KAACORE_ASSERT(this->_font._font_data);
    this->_render_glyphs = this->_font._font_data->generate_render_glyphs(
        this->_content, this->_font_size);
    FontRenderGlyph::arrange_glyphs(
        this->_render_glyphs, this->_first_line_indent,
        this->_font_size * this->_interline_spacing, this->_line_width);

    Node* node = container_node(this);
    node->sprite(this->_font._font_data->baked_texture);
    node->shape(FontRenderGlyph::make_shape(this->_render_glyphs));
}

std::string
TextNode::content() const
{
    return this->_content;
}

void
TextNode::content(const std::string& content)
{
    this->_content = content;
    this->_update_shape();
}

double
TextNode::font_size() const
{
    return this->_font_size;
}

void
TextNode::font_size(const double font_size)
{
    this->_font_size = font_size;
    this->_update_shape();
}

double
TextNode::line_width() const
{
    return this->_line_width;
}

void
TextNode::line_width(const double line_width)
{
    this->_line_width = line_width;
    this->_update_shape();
}

double
TextNode::interline_spacing() const
{
    return this->_interline_spacing;
}

void
TextNode::interline_spacing(const double interline_spacing)
{
    this->_interline_spacing = interline_spacing;
    this->_update_shape();
}

double
TextNode::first_line_indent() const
{
    return this->_first_line_indent;
}

void
TextNode::first_line_indent(const double first_line_indent)
{
    this->_first_line_indent = first_line_indent;
    this->_update_shape();
}

Font
TextNode::font() const
{
    return this->_font;
}

void
TextNode::font(const Font& font)
{
    this->_font = font;
    this->_update_shape();
}

} // namespace kaacore
