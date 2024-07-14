#pragma once

#include <cmath>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "stb_truetype.h"

#include "kaacore/files.h"
#include "kaacore/memory.h"
#include "kaacore/renderer.h"
#include "kaacore/resources.h"
#include "kaacore/shapes.h"
#include "kaacore/textures.h"

namespace kaacore {

void
initialize_fonts();
void
uninitialize_fonts();

typedef std::vector<stbtt_packedchar> BakedFontData;
typedef uint32_t UnicodeCodepoint;

// width of created font atlas texture
const size_t font_baker_texture_width = 2048;
// max height of created font atlas texture,
// it will be trimmed to optimal after completion
const size_t font_baker_texture_max_height = 10240;
const size_t font_baker_pixel_height = 80;
const UnicodeCodepoint font_baker_first_glyph = 32;
const size_t font_baker_glyphs_count = 96;

// padding field added around glyph
const int font_sdf_padding = 5;

// value of SDF exactly on glyphs edge
const int font_sdf_edge_value = 180;

// how much field value increases/decreases over 1 pixel
const float font_sdf_pixel_dist_scale =
    font_sdf_edge_value / double(font_sdf_padding);

struct FontMetrics {
    FontMetrics() = default;
    inline FontMetrics(
        const double ascent, const double descent, const double line_gap
    )
        : ascent(ascent), descent(descent), line_gap(line_gap)
    {}
    FontMetrics scale_for_pixel_height(const double font_pixel_height) const;
    double height() const;

    double ascent;
    double descent;
    double line_gap;
};

struct FontRenderGlyph {
    UnicodeCodepoint codepoint;
    glm::dvec2 offset;
    glm::dvec2 size;
    glm::dvec2 position;
    glm::dvec2 texture_uv0;
    glm::dvec2 texture_uv1;
    double advance;

    FontRenderGlyph(
        UnicodeCodepoint codepoint, stbtt_packedchar glyph_data,
        double scale_factor, const glm::dvec2 inv_texture_size
    );
    FontRenderGlyph(
        UnicodeCodepoint codepoint, stbtt_packedchar glyph_data,
        double scale_factor, const glm::dvec2 inv_texture_size,
        const FontRenderGlyph& other_glyph
    );

    bool has_size() const;

    static void arrange_glyphs(
        std::vector<FontRenderGlyph>& render_glyphs, const double indent,
        const double line_height, const double line_width = INFINITY
    );
    static Shape make_shape(
        const std::vector<FontRenderGlyph>& render_glyphs,
        const FontMetrics font_metrics
    );
};

class FontData : public Resource {
  public:
    const std::string path;
    BakedFontData baked_font;
    ResourceReference<Texture> baked_texture;
    FontMetrics font_metrics;

    ~FontData();
    static ResourceReference<FontData> load(const std::string& path);
    static ResourceReference<FontData> load_from_memory(const Memory& memory);
    Shape generate_text_shape(
        const std::string& text, double size, double indent, double max_width
    );
    std::vector<FontRenderGlyph> generate_render_glyphs(
        const std::string& text, const double scale_factor
    );
    inline FontMetrics metrics() { return this->font_metrics; }

  private:
    FontData(const std::string& path);
    FontData(
        const ResourceReference<Texture> baked_texture,
        const BakedFontData baked_font, const FontMetrics font_metrics
    );
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
} // namespace std
