#include "FontSystem.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <freetype/freetype.h>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <unicode/uchar.h>
#include <unicode/uscript.h>
#include <unicode/utf8.h>
#include <utility>
#include <vector>
#include <unicode/ubidi.h>

#include <harfbuzz/hb-ft.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

using namespace std;

namespace {
    FT_Library ft_lib = nullptr;
    mutex ft_lib_mutex;
}

size_t hash<FontSystem::CacheKey>::operator()(const FontSystem::CacheKey &key) const noexcept
{
    return hash<void *>()(key.face) ^ (hash<float>()(key.font_size) << 1) ^ (hash<uint16_t>()(key.glyph_id) << 2);
}



FontSystem::FontSystem(const std::vector<MetaFace> &fontFaces)
    : atlas(1024, 1024)
{
    lock_guard<mutex> ft_lib_mutex_lock{ft_lib_mutex};
    if(!ft_lib)
        assert(FT_Init_FreeType(&ft_lib));
}


FontSystem::Face &FontSystem::Font::Normal()
{
    if(normal)
        return *normal;
    if(bold)
        return *bold;
    if(italic)
        return *italic;

    throw runtime_error("Font family " + family_name + " does not have any fonts");
}



std::pair<FontSystem::Face &, bool> FontSystem::Font::Italic()
{
    if(italic)
        return {*italic, false};
    if(normal)
        return {*normal, true};
    if(bold)
        return {*bold, true};

    throw runtime_error("Font family " + family_name + " does not have any fonts");
}



std::pair<FontSystem::Face &, bool> FontSystem::Font::Bold()
{
    if(bold)
        return {*bold, false};
    if(normal)
        return {*normal, true};
    if(italic)
        return {*italic, true};

    throw runtime_error("Font family " + family_name + " does not have any fonts");
}



FontSystem::Placement FontSystem::GetPlacement(const CacheKey &key)
{
    if(atlasMap.count(key))
        return atlasMap[key];

    auto face =reinterpret_cast<Face *>(key.face);
    FT_Set_Char_Size(face->ft, key.font_size * 64, 0, 0, 0);
    FT_Load_Glyph(face->ft, key.glyph_id, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD);

    auto rect = stbrp_rect {
        key.glyph_id,
        static_cast<int>(face->ft->glyph->bitmap.width / 3),
        static_cast<int>(face->ft->glyph->bitmap.rows),
        0,
        0,
        0
    };

    if(!stbrp_pack_rects(&atlas.context, &rect, 1))
    {
        atlas = GlyphAtlas(atlas.context.width * 1.2, atlas.context.height * 1.2);

        return Placement {
            rect,
            GlyphMetrics {
                face->ft->glyph->metrics
            }
        };
    }

    GLuint tex;
	GLuint buf;
	glGenFramebuffers(1, &buf);
	glBindFramebuffer(GL_FRAMEBUFFER, buf);
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rect.w, rect.h, 0, GL_RGB, GL_UNSIGNED_BYTE, face->ft->glyph->bitmap.buffer);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, buf);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, atlas.buffer);

	glBlitFramebuffer(0, 0, rect.w, rect.h, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glDeleteFramebuffers(1, &buf);
	glDeleteTextures(1, &tex);

	atlasMap[key] = {rect, {face->ft->glyph->metrics}};

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return {rect, {face->ft->glyph->metrics}};
}



FontSystem::GlyphAtlas::GlyphAtlas(int width, int height)
{
    nodes = vector<stbrp_node>(width);
    assert(nodes.size() == width);
    stbrp_init_target(&context, width, height, nodes.data(), nodes.size());

    glGenFramebuffers(1, &buffer);
    glBindFramebuffer(GL_FRAMEBUFFER, buffer);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    GLenum draw_buffer[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, draw_buffer);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}



FontSystem::Face::Face(const filesystem::path &path, int index)
{
    lock_guard<mutex> ft_lib_mutex_lock{ft_lib_mutex};
    assert(FT_New_Face(ft_lib, path.c_str(), index, &ft));
    hb_face = hb_ft_face_create_referenced(ft);
}



FontSystem::Face::~Face()
{
    lock_guard<mutex> ft_lib_mutex_lock{ft_lib_mutex};
    FT_Done_Face(ft);
    hb_face_destroy(hb_face);
}



FontSystem::GlyphAtlas::~GlyphAtlas()
{
    glDeleteFramebuffers(1, &buffer);
    glDeleteTextures(1, &tex);
}



FontSystem::GlyphAtlas &FontSystem::Atlas()
{
    return atlas;
}



int FontSystem::Width(const std::string &text, const Attributes &attrs, float font_size)
{
    auto &cached = GetCachedGlyphs(text, attrs);

    int cursor = 0;
    int max_width = 0;
    for(const auto &sg : cached)
    {
        max_width = max(static_cast<int>(GetPlacement(CacheKey(sg.face, sg.glyph_id, font_size)).metrics.metrics.width) + cursor, max_width);
        cursor += sg.x_advance;
    }

    return max_width;
}



int FontSystem::WidthSpans(const std::vector<std::pair<std::string, Attributes>> &line, float font_size)
{
    int cursor = 0;
    int max_width = 0;

    for(const auto &span : line)
    {
        auto &cached = GetCachedGlyphs(span.first, span.second);

        for(const auto &sg : cached)
        {
            max_width = max(static_cast<int>(GetPlacement(CacheKey(sg.face, sg.glyph_id, font_size)).metrics.metrics.width) + cursor, max_width);
            cursor += sg.x_advance;
        }
    }
    return max_width;
}



vector<UScriptCode> FontSystem::LocaleFallbacksNeeded(const std::string &str)
{
    vector<UScriptCode> scripts;

    const char *s = str.data();
    int length = str.length();
    for(int i = 0; i < length;)
    {
        UChar32 c;
        U8_NEXT(s, i, length, c);
        auto script = u_getIntPropertyValue(c, UProperty::UCHAR_SCRIPT);
        if(std::find(scripts.cbegin(), scripts.cend(), script) != scripts.cend())
            // More of the same script is probably going to follow a character of a certain script
            scripts.insert(scripts.cbegin(), static_cast<UScriptCode>(script));
    }

    return scripts;
}



vector<size_t> FontSystem::ShapeFallback(vector<ShapeGlyph> &glyphs, Font &font, u32string_view line, const Attributes &attrs)
{

}
