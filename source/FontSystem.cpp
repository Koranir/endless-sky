#include "FontSystem.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <unicode/uchar.h>
#include <unicode/uscript.h>
#include <unicode/utf8.h>
#include <utility>
#include <vector>

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
