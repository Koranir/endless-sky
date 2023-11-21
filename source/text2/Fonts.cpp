#include "./Fonts.h"

#include <atomic>
#include <optional>
#include <map>

using namespace std;

namespace {
    map<Font2s::ID, Font2> fonts;
    atomic_uint_fast32_t lim = 0;

    Font2 &LoadFontFromSystem(uint32_t glyphID) {
        
    }
}

Font2s::ID::ID() {
    fontID = lim.fetch_add(1, std::memory_order_relaxed);
}

Font2s::ID Font2s::Load(const vector<char> &data, uint32_t size) {
    const auto id = ID();
    fonts[id] = Font2(data, size);
    return id;
}

Font2 &Font2s::MatchGlyph(uint32_t glyph) {
    for(auto &id_font : fonts) if(id_font.second.Has(glyph)) return id_font.second;
    return LoadFontFromSystem(glyph);
}
