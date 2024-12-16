#include <cstring>
#include <memory>
#include <string_view>
#include <vector>

#include "stb_rect_pack.h"
#include "stb_truetype.h"

#include "kaacore/embedded_data.h"
#include "kaacore/engine.h"
#include "kaacore/exceptions.h"
#include "kaacore/fonts.h"
#include "kaacore/nodes.h"
#include "kaacore/textures.h"
#include "kaacore/unicode_buffer.h"
#include "kaacore/utils.h"

#include "kaacore/fonts.h"

using namespace std::literals::string_view_literals;

namespace kaacore {

ResourcesRegistry<FontResourcesRegistryKey, FontData> _fonts_registry;

constexpr auto default_codepoints =
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"sv;

// SDF rasterization helper struct
struct GlyphSDF {
    UnicodeCodepoint codepoint;
    glm::ivec2 dimensions;
    glm::ivec2 offset;
    double scale;
    std::unique_ptr<unsigned char[], std::function<void(unsigned char*)>> data;
    BitmapView<> bitmap_view;

    GlyphSDF(
        const stbtt_fontinfo& font_info, const UnicodeCodepoint codepoint,
        const double scale
    )
        : codepoint(codepoint), data(nullptr), dimensions({0, 0}),
          offset({0, 0}), scale(scale)
    {
        auto raw_data = stbtt_GetCodepointSDF(
            &font_info, scale, codepoint, font_sdf_padding, font_sdf_edge_value,
            font_sdf_pixel_dist_scale, &dimensions.x, &dimensions.y, &offset.x,
            &offset.y
        );
        this->data = {raw_data, [](unsigned char* sdf_data) {
                          stbtt_FreeSDF(sdf_data, nullptr);
                      }};
        if (this->data != nullptr) {
            this->bitmap_view = BitmapView{this->data.get(), this->dimensions};
        }
    }

    operator bool() const { return this->data != nullptr; }
};

void
initialize_fonts()
{
    _fonts_registry.initialze();
    auto& default_font = get_default_font();
    if (not default_font._font_data.res_ptr.get()->is_initialized) {
        default_font._font_data.res_ptr.get()->_initialize();
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

std::vector<GlyphSDF>
_rasterize_sdf_bitmaps(
    const stbtt_fontinfo& font_info, const double font_baking_scale,
    const UnicodeView& codepoints
)
{
    std::vector<GlyphSDF> sdfs;

    for (UnicodeCodepoint base_codepoint : default_codepoints) {
        auto& sdf =
            sdfs.emplace_back(font_info, base_codepoint, font_baking_scale);
        KAACORE_LOG_TRACE(
            "Generated SDF glyph for base codepoint: {:#x}, dimensions: ({}, "
            "{}), "
            "offset: ({}, {})",
            base_codepoint, sdf.dimensions.x, sdf.dimensions.y, sdf.offset.x,
            sdf.offset.y
        );
    }

    for (UnicodeCodepoint additional_codepoint : codepoints) {
        auto& sdf = sdfs.emplace_back(
            font_info, additional_codepoint, font_baking_scale
        );
        KAACORE_LOG_TRACE(
            "Generated SDF glyph for additional codepoint: {:#x}, dimensions: "
            "({}, {}), "
            "offset: ({}, {})",
            additional_codepoint, sdf.dimensions.x, sdf.dimensions.y,
            sdf.offset.x, sdf.offset.y
        );
    }

    return std::move(sdfs);
}

std::pair<bimg::ImageContainer*, BakedFontData>
_pack_sdf_glyphs(
    const stbtt_fontinfo& font_info, const std::vector<GlyphSDF>& sdfs
)
{
    BakedFontData baked_font_data;
    std::vector<stbrp_rect> rects;
    baked_font_data.reserve(sdfs.size());
    rects.reserve(sdfs.size());

    for (const auto& sdf : sdfs) {
        auto& rect = rects.emplace_back();
        rect.w = sdf.dimensions.x;
        rect.h = sdf.dimensions.y;
    }

    stbtt_pack_context pack_ctx;
    stbtt_PackBegin(
        &pack_ctx, nullptr, font_baker_texture_width,
        font_baker_texture_max_height, 0, 1, nullptr
    );
    stbtt_PackFontRangesPackRects(&pack_ctx, rects.data(), rects.size());
    stbtt_PackEnd(&pack_ctx);

    auto lowest_rect = *std::max_element(
        rects.begin(), rects.end(),
        [](const auto& a, const auto& b) { return a.y + a.h < b.y + b.h; }
    );
    auto actual_texture_height = lowest_rect.y + lowest_rect.h;
    KAACORE_LOG_DEBUG(
        "Generated font atlas size: ({}, {})", font_baker_texture_width,
        actual_texture_height
    );
    Bitmap atlas_bitmap{{font_baker_texture_width, actual_texture_height}};

    for (int i = 0; i < sdfs.size(); i++) {
        auto& rect = rects[i];
        auto& sdf = sdfs[i];

        int horizontal_advance, left_side_bearing;
        stbtt_GetCodepointHMetrics(
            &font_info, sdf.codepoint, &horizontal_advance, &left_side_bearing
        );

        stbtt_packedchar glyph_data;
        glyph_data.x0 = rect.x;
        glyph_data.y0 = rect.y;
        glyph_data.x1 = rect.x + rect.w;
        glyph_data.y1 = rect.y + rect.h;
        glyph_data.xoff = sdf.offset.x;
        glyph_data.yoff = sdf.offset.y;
        glyph_data.xoff2 = sdf.offset.x + rect.w;
        glyph_data.yoff2 = sdf.offset.y + rect.h;
        glyph_data.xadvance = horizontal_advance * sdf.scale;

        baked_font_data.emplace(sdf.codepoint, glyph_data);

        if (sdf) {
            atlas_bitmap.blit(sdf.bitmap_view, {rect.x, rect.y});
        }
    }

    bimg::ImageContainer* baked_font_image = load_raw_image(
        bimg::TextureFormat::Enum::R8, atlas_bitmap.dimensions.x,
        atlas_bitmap.dimensions.y, atlas_bitmap.container
    );

    return {baked_font_image, baked_font_data};
}

std::tuple<bimg::ImageContainer*, BakedFontData, FontMetrics>
bake_font_texture(
    const uint8_t* font_file_content, const size_t size,
    const UnicodeView& codepoints
)
{
    if (memcmp(
            font_file_content, "\x00\x01\x00\x00\x00", size >= 5 ? 5 : size
        ) != 0) {
        throw kaacore::exception(
            "Provided file is not TTF - magic number mismatched."
        );
    }

    KAACORE_LOG_DEBUG("Loading font data.");
    stbtt_fontinfo font_info;
    font_info.userdata = nullptr;
    stbtt_InitFont(
        &font_info, font_file_content,
        stbtt_GetFontOffsetForIndex(font_file_content, 0)
    );

    const double font_baking_scale =
        stbtt_ScaleForPixelHeight(&font_info, font_baker_pixel_height);
    KAACORE_LOG_DEBUG(
        "Calculated font baking scale: {:.4f}", font_baking_scale
    );

    auto sdf_bitmaps =
        _rasterize_sdf_bitmaps(font_info, font_baking_scale, codepoints);
    auto [image_container, baked_font_data] =
        _pack_sdf_glyphs(font_info, sdf_bitmaps);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
    FontMetrics font_metrics{
        static_cast<double>(ascent), static_cast<double>(descent),
        static_cast<double>(line_gap)
    };
    KAACORE_LOG_DEBUG(
        "Font metrics - ascent: {:.2f}, descent: {:.2f}, line gap: {:.2f}",
        font_metrics.ascent, font_metrics.descent, font_metrics.line_gap
    );

    return {image_container, baked_font_data, font_metrics};
}

std::tuple<bimg::ImageContainer*, BakedFontData, FontMetrics>
bake_font_texture(
    const File& font_file, const UnicodeView& additional_codepoints
)
{
    return bake_font_texture(
        font_file.content.data(), font_file.content.size(),
        additional_codepoints
    );
}

FontMetrics
FontMetrics::scale_for_pixel_height(const double font_pixel_height) const
{
    const double scaling = font_pixel_height / this->height();
    return {
        this->ascent * scaling, this->descent * scaling,
        this->line_gap * scaling
    };
}

double
FontMetrics::height() const
{
    return (this->ascent - this->descent);
}

FontRenderGlyph::FontRenderGlyph(
    UnicodeCodepoint codepoint, stbtt_packedchar glyph_data,
    double scale_factor, const glm::dvec2 inv_texture_size
)
    : codepoint(codepoint), position(0., 0.)
{
    this->offset = glm::dvec2(glyph_data.xoff, glyph_data.yoff) * scale_factor;
    this->size = glm::dvec2(
                     glyph_data.xoff2 - glyph_data.xoff,
                     glyph_data.yoff2 - glyph_data.yoff
                 ) *
                 scale_factor;
    this->advance = glyph_data.xadvance * scale_factor;

    this->texture_uv0 =
        glm::dvec2(glyph_data.x0, glyph_data.y0) * inv_texture_size;
    this->texture_uv1 =
        glm::dvec2(glyph_data.x1, glyph_data.y1) * inv_texture_size;
    KAACORE_LOG_TRACE(
        "Glyph: {:#x}, size: ({}, {}), offset: ({}, {}), "
        "advance: {:.2f}, texture_uv0: ({:.2f}, {:.2f}), "
        "texture_uv1: ({:.2f}, {:.2f})",
        codepoint, this->size.x, this->size.y, this->offset.x, this->offset.y,
        this->advance, this->texture_uv0.x, this->texture_uv0.y,
        this->texture_uv1.x, this->texture_uv1.y
    );
}

FontRenderGlyph::FontRenderGlyph(
    UnicodeCodepoint codepoint, stbtt_packedchar glyph_data,
    double scale_factor, const glm::dvec2 inv_texture_size,
    const FontRenderGlyph& other_glyph
)
    : FontRenderGlyph(codepoint, glyph_data, scale_factor, inv_texture_size)
{
    this->position.x = other_glyph.position.x + other_glyph.advance;
}

bool
FontRenderGlyph::has_size() const
{
    return this->size.x > 0 and this->size.y > 0;
}

void
FontRenderGlyph::arrange_glyphs(
    std::vector<FontRenderGlyph>& render_glyphs, const double indent,
    const double line_height, const double line_width
)
{
    glm::dvec2 current_pos = {indent, 0.};
    std::vector<FontRenderGlyph>::iterator word_start = render_glyphs.begin();

    for (auto it = word_start; it != render_glyphs.end(); it++) {
        if (it->codepoint == static_cast<UnicodeCodepoint>(' ')) {
            word_start = it + 1;
            if (current_pos.x == 0.) {
                continue;
            }
        }
        it->position = current_pos;

        if (it->codepoint == static_cast<UnicodeCodepoint>('\n')) {
            word_start = it + 1;
            current_pos.x = 0.;
            current_pos.y += line_height;
            continue;
        }
        current_pos.x += it->advance;

        if (current_pos.x > line_width and
            ((it + 1) == render_glyphs.end() or
             (it + 1)->codepoint == static_cast<UnicodeCodepoint>(' '))) {
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
FontRenderGlyph::make_shape(
    const std::vector<FontRenderGlyph>& render_glyphs,
    const FontMetrics font_metrics
)
{
    if (render_glyphs.empty()) {
        return Shape{};
    }

    std::vector<StandardVertexData> vertices;
    std::vector<VertexIndex> indices;
    BoundingBox<double> bounding_box;

    auto glyphs_count = render_glyphs.size();

    vertices.reserve(glyphs_count * 4);
    indices.reserve(glyphs_count * 6);

    for (const FontRenderGlyph& rg : render_glyphs) {
        BoundingBox<double> rg_bounding_box{
            rg.position.x, rg.position.y - font_metrics.ascent,
            rg.position.x + rg.advance, rg.position.y - font_metrics.descent
        };
        bounding_box = rg_bounding_box.merge(bounding_box);

        if (not rg.has_size()) {
            continue;
        }
        auto vertices_count = vertices.size();

        vertices.push_back(
            // Left-top vertex
            StandardVertexData::xy_uv(
                rg.position.x + rg.offset.x, rg.position.y + rg.offset.y,
                rg.texture_uv0.x, rg.texture_uv0.y
            )
        );
        vertices.push_back(
            // Right-top vertex
            StandardVertexData::xy_uv(
                rg.position.x + rg.offset.x + rg.size.x,
                rg.position.y + rg.offset.y, rg.texture_uv1.x, rg.texture_uv0.y
            )
        );
        vertices.push_back(
            // Left-bottom vertex
            StandardVertexData::xy_uv(
                rg.position.x + rg.offset.x,
                rg.position.y + rg.offset.y + rg.size.y, rg.texture_uv0.x,
                rg.texture_uv1.y
            )
        );
        vertices.push_back(
            // Right-bottom vertex
            StandardVertexData::xy_uv(
                rg.position.x + rg.offset.x + rg.size.x,
                rg.position.y + rg.offset.y + rg.size.y, rg.texture_uv1.x,
                rg.texture_uv1.y
            )
        );

        indices.push_back(vertices_count + 0);
        indices.push_back(vertices_count + 2);
        indices.push_back(vertices_count + 1);
        indices.push_back(vertices_count + 1);
        indices.push_back(vertices_count + 2);
        indices.push_back(vertices_count + 3);
    }

    return Shape::Freeform(indices, vertices, bounding_box);
}

FontData::FontData(
    const std::string& path, const UnicodeView additional_codepoints
)
    : path(path), additional_codepoints(additional_codepoints)
{
    File file(this->path);
    auto [baked_font_image, baked_font_data, font_metrics] =
        bake_font_texture(file, additional_codepoints);
    this->baked_texture = MemoryTexture::create(baked_font_image);
    this->baked_font = std::move(baked_font_data);
    this->font_metrics = font_metrics;

    if (is_engine_initialized()) {
        this->_initialize();
    }
}

FontData::FontData(
    const ResourceReference<Texture> baked_texture,
    const BakedFontData baked_font, const FontMetrics font_metrics
)
{
    this->baked_texture = baked_texture;
    this->baked_font = std::move(baked_font);
    this->font_metrics = font_metrics;

    if (is_engine_initialized()) {
        this->_initialize();
    }
}

ResourceReference<FontData>
FontData::load(const std::string& path, const UnicodeView additional_codepoints)
{
    std::shared_ptr<FontData> font_data;
    if ((font_data =
             _fonts_registry.get_resource({path, additional_codepoints}))) {
        return font_data;
    }

    font_data =
        std::shared_ptr<FontData>(new FontData(path, additional_codepoints));
    _fonts_registry.register_resource({path, additional_codepoints}, font_data);
    return font_data;
}

ResourceReference<FontData>
FontData::load_from_memory(const Memory& memory)
{
    auto raw_memory = reinterpret_cast<const uint8_t*>(memory.get());
    auto [baked_font_texture, baked_font_data, font_metrics] =
        bake_font_texture(raw_memory, memory.size(), UnicodeView{});

    return std::shared_ptr<FontData>(new FontData(
        MemoryTexture::create(baked_font_texture), baked_font_data, font_metrics
    ));
}

FontData::~FontData()
{
    if (this->is_initialized) {
        this->_uninitialize();
    }
}

std::vector<FontRenderGlyph>
FontData::generate_render_glyphs(
    const UnicodeView text, const double scale_factor
)
{
    std::vector<FontRenderGlyph> render_glyphs;
    const glm::dvec2 inv_texture_size = {
        1. / this->baked_texture->get_dimensions().x,
        1. / this->baked_texture->get_dimensions().y
    };

    for (UnicodeCodepoint codepoint : text) {
        if (codepoint == '\n') {
            codepoint = static_cast<UnicodeCodepoint>(' ');
        }

        auto it = this->baked_font.find(codepoint);
        stbtt_packedchar glyph_data;
        if (it != this->baked_font.end()) {
            glyph_data = it->second;
        } else {
            KAACORE_LOG_WARN("Unhadled font character: {:#x}", codepoint);
            glyph_data =
                this->baked_font.at(static_cast<UnicodeCodepoint>('?'));
        }

        if (not render_glyphs.empty()) {
            render_glyphs.emplace_back(
                codepoint, glyph_data, scale_factor, inv_texture_size,
                render_glyphs.back()
            );
        } else {
            render_glyphs.emplace_back(
                codepoint, glyph_data, scale_factor, inv_texture_size
            );
        }
    }

    return render_glyphs;
}

void
FontData::_initialize()
{
    if (not this->baked_texture.res_ptr.get()->is_initialized) {
        this->baked_texture.res_ptr.get()->_initialize();
    }
    this->is_initialized = true;
}

void
FontData::_uninitialize()
{
    if (this->baked_texture.res_ptr.get()->is_initialized) {
        this->baked_texture.res_ptr.get()->_uninitialize();
    }
    this->is_initialized = false;
}

Font::Font() {}

Font::Font(const ResourceReference<FontData>& font_data) : _font_data(font_data)
{}

Font
Font::load(const std::string& font_filepath)
{
    return Font(FontData::load(font_filepath, UnicodeView{}));
}

Font
Font::load(
    const std::string& font_filepath, const UnicodeView additional_codepoints
)
{
    return Font(FontData::load(font_filepath, additional_codepoints));
}

bool
Font::operator==(const Font& other)
{
    return this->_font_data == other._font_data;
}

Font&
get_default_font()
{
    static auto memory = get_embedded_file_content(
        embedded_assets_filesystem, "embedded_resources/font_munro/munro.ttf"
    );
    static Font default_font{FontData::load_from_memory(memory)};
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
    KAACORE_ASSERT(this->_font._font_data, "Invalid internal font state.");
    const double scale_factor = this->_font_size / font_baker_pixel_height;
    const auto scaled_metrics =
        this->_font._font_data->metrics().scale_for_pixel_height(
            this->_font_size
        );
    this->_render_glyphs = this->_font._font_data->generate_render_glyphs(
        this->_content.view(), scale_factor
    );
    FontRenderGlyph::arrange_glyphs(
        this->_render_glyphs, this->_first_line_indent,
        this->_font_size * this->_interline_spacing, this->_line_width
    );

    Node* node = container_node(this);
    node->sprite(this->_font._font_data->baked_texture);
    node->shape(
        FontRenderGlyph::make_shape(this->_render_glyphs, scaled_metrics)
    );
}

const UnicodeView
TextNode::content() const
{
    return this->_content.view();
}

void
TextNode::content(const UnicodeView content)
{
    this->_content = content;
    this->_update_shape();
}

void
TextNode::content(const std::string& content)
{
    this->content(UnicodeView{content});
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
