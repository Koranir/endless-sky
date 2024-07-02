#include "FontSystem.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <functional>
#include <stdexcept>
#include <unicode/uchar.h>
#include <unicode/uscript.h>
#include <unicode/utf8.h>
#include <utility>
#include <vector>

#include <harfbuzz/hb-ft.h>

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

using namespace std;

size_t hash<FontSystem::CacheKey>::operator()(const FontSystem::CacheKey &key) const noexcept
{
    return hash<void *>()(key.face) ^ (hash<float>()(key.font_size) << 1) ^ (hash<uint16_t>()(key.glyph_id) << 2);
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



FontSystem::Placement &FontSystem::GetPlacement(const CacheKey &key)
{
    return atlasMap[key];
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
    hb_blob = hb_blob_create_from_file(path.c_str());
    hb_face = hb_face_create(hb_blob, index);
    hb = hb_font_create(hb_face);

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
