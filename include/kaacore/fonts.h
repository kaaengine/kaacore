#pragma once

#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "stb_truetype.h"

#include "kaacore/files.h"
#include "kaacore/images.h"
#include "kaacore/renderer.h"
#include "kaacore/resources.h"
#include "kaacore/shapes.h"

namespace kaacore {

void
initialize_fonts();
void
uninitialize_fonts();

typedef std::vector<stbtt_packedchar> BakedFontData;

const size_t font_baker_texture_size = 2048;
const double font_baker_inverted_texture_size = 1. / font_baker_texture_size;
const size_t font_baker_pixel_height = 80;
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

    FontRenderGlyph(
        uint32_t character, stbtt_packedchar glyph_data, double scale_factor);
    FontRenderGlyph(
        uint32_t character, stbtt_packedchar glyph_data, double scale_factor,
        const FontRenderGlyph& other_glyph);

    static void arrange_glyphs(
        std::vector<FontRenderGlyph>& render_glyphs, const double indent,
        const double line_height, const double line_width = INFINITY);
    static Shape make_shape(const std::vector<FontRenderGlyph>& render_glyphs);
};

class FontData : public Resource {
  public:
    const std::string path;
    BakedFontData baked_font;
    ResourceReference<Image> baked_texture;

    ~FontData();
    static ResourceReference<FontData> load(const std::string& path);
    static ResourceReference<FontData> load_from_memory(
        const uint8_t* font_file_content, const size_t size);
    Shape generate_text_shape(
        const std::string& text, double size, double indent, double max_width);
    std::vector<FontRenderGlyph> generate_render_glyphs(
        const std::string& text, const double scale_factor);

  private:
    FontData(const std::string& path);
    FontData(
        const ResourceReference<Image> baked_texture,
        const BakedFontData baked_font);
    virtual void _initialize() override;
    virtual void _uninitialize() override;

    friend class ResourcesRegistry<std::string, FontData>;
    friend void initialize_fonts();
    friend void uninitialize_fonts();
};

class TextNode;

class Font {
  public:
    Font();
    static Font load(const std::string& font_filepath);

    bool operator==(const Font& other);

  private:
    ResourceReference<FontData> _font_data;

    Font(const ResourceReference<FontData>& font_data);

    friend class TextNode;
    friend std::hash<Font>;
    friend void initialize_fonts();
    friend void uninitialize_fonts();
    friend Font& get_default_font();
};

Font&
get_default_font();

class TextNode {
    std::string _content;
    double _font_size;
    double _line_width;
    double _interline_spacing;
    double _first_line_indent;
    Font _font;

    std::vector<FontRenderGlyph> _render_glyphs;

    void _update_shape();

  public:
    TextNode();
    ~TextNode();

    std::string content() const;
    void content(const std::string& content);

    double font_size() const;
    void font_size(const double font_size);

    double line_width() const;
    void line_width(const double line_width);

    double interline_spacing() const;
    void interline_spacing(const double interline_spacing);

    double first_line_indent() const;
    void first_line_indent(const double first_line_indent);

    Font font() const;
    void font(const Font& font);
};

} // namespace kaacore

namespace std {
using kaacore::Font;
using kaacore::FontData;

template<>
struct hash<kaacore::Font> {
    size_t operator()(const Font& font) const
    {
        return std::hash<ResourceReference<FontData>>{}(font._font_data);
    }
};
}
