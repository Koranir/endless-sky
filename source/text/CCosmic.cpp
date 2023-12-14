#include "CCosmic.h"

#include "../Files.h"
#include "../Preferences.h"
#include "../Screen.h"
#include "Utf8.h"
#include "alignment.hpp"

#include <SDL_video.h>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <locale>
#include <string>
#include <tuple>
#include <unordered_map>
#include <memory>
#include <utility>
#include <vector>

#include "../Messages.h"

using namespace std;

struct ShapeKey {
    ShapeKey(const string &text, const float fontSize, const float lineHeight, const float bufWidth, const float bufHeight, const Alignment align)
        : text(text), fontSize(fontSize), lineHeight(lineHeight), bufWidth(bufWidth), bufHeight(bufHeight), align(align) {}

    bool operator==(const ShapeKey &other) const
    {
        return text == other.text && fontSize == other.fontSize && lineHeight == other.lineHeight && align == other.align;
    }

    const string text;
    const float fontSize;
    const float lineHeight;
    const float bufWidth;
    const float bufHeight;
    const Alignment align;

};

template<>
struct std::hash<ShapeKey> 
{
    size_t operator()(const ShapeKey &other) const {
        return (((((hash<string>()(other.text)
             ^ (hash<float>()(other.fontSize) << 1)) >> 1)
             ^ (hash<float>()(other.lineHeight) << 1) >> 2)
             ^ (hash<float>()(other.bufWidth) << 1) >> 3)
             ^ (hash<float>()(other.bufHeight) << 1) >> 4)
             ^ (hash<Alignment>()(other.align) << 1) >> 5;
    }
};

namespace {
    FfiCtr ctr;

    struct CCBuffer {
        CCBuffer() {}
        CCBuffer(CtrBuffer buf) : buf(buf) {}
        CCBuffer(const CCBuffer &) = delete;
        CCBuffer &operator= (const CCBuffer &) = delete;
        CCBuffer &operator= (CCBuffer &&) = default;
        ~CCBuffer() { CtrBufferDrop(buf); }
        operator CtrBuffer() { return buf; }

        CtrBuffer buf;
    };

    class ExceptGuard {
    public:
        ExceptGuard() {}
        void Set(FfiCtr ptr) { ctrptr = ptr; }
        ~ExceptGuard() { if(ctrptr) ctrDeinit(ctrptr); }

    private:
        FfiCtr ctrptr = nullptr;
    };
    ExceptGuard _ctrGuard;

    unordered_map<ShapeKey, pair<unique_ptr<CCBuffer>, bool>> shapedCache;

    bool show_underlines = false;

    CtrBuffer get_cached(const string &text, const float fontSize, const float lineHeight, const float bufWidth, const float bufHeight, const Alignment align)
    {
        const auto key = ShapeKey(text, fontSize, lineHeight, bufWidth, bufHeight, align);
        const auto maybe_buf = shapedCache.find(key);

        if(maybe_buf == shapedCache.end())
        {
            shapedCache[key] = make_pair(unique_ptr<CCBuffer>(new CCBuffer(ctrCreateBuffer(ctr, fontSize, lineHeight))), true);
            CtrBuffer buf = shapedCache[key].first->buf;
            ctrSetBufferSize(ctr, buf, bufWidth, bufHeight );
            ctrSetBufferContents(ctr, buf, reinterpret_cast<const uint8_t *>(text.data()), text.size());
            CtrAlign alignc;
            switch (align) {
            case Alignment::CENTER:
                alignc = CtrAlign::CENTER;
                break;
            case Alignment::JUSTIFIED:
                alignc = CtrAlign::JUSTIFIED;
                break;
            case Alignment::LEFT:
                alignc = CtrAlign::LEFT;
                break;
            case Alignment::RIGHT:
                alignc = CtrAlign::RIGHT;
                break;
            }
            ctrShapeBuffer(ctr, buf, alignc);
            return buf;
        }
        else
        {
            maybe_buf->second.second = true;
            return maybe_buf->second.first->buf;
        }
    }
}

void CCosmicText::Init(const vector<const char *> &fonts)
{
    ctr = ctrInit(Files::Fonts().c_str(), &SDL_GL_GetProcAddress, fonts.data(), fonts.size());
    _ctrGuard.Set(ctr);
}

string CCosmicText::Debug()
{
    auto ptr = ctrDebug(ctr);
    auto str = string(ptr);
    ctrDeinitDebug(ptr);
    return str;
}

void CCosmicText::ClearCache()
{
    vector<ShapeKey> toRemove;
    for(auto &it : shapedCache)
    {
        if(!it.second.second) {
            toRemove.emplace_back(it.first);
        }
        it.second.second = false;
    }
    for(const auto &it : toRemove)
    {
        shapedCache.erase(it);
    }
}

void CCosmicText::DirectDrawText(const std::string &text, Rectangle where, float fontSize, float lineHeight, const Color &col, Alignment align)
{
    CtrBuffer buf = get_cached(text, fontSize, lineHeight, where.Width(), where.Height(), align);

    ctrDrawBuffer(ctr, buf, col.Argb8(), CtrRect {
        {static_cast<float>(where.Left()), static_cast<float>(where.Bottom())},
        {static_cast<float>(where.Right()), static_cast<float>(where.Top())}
    }, Screen::Width(), Screen::Height());

    Messages::Add(to_string(shapedCache.size() * (sizeof(ShapeKey) + sizeof(CtrBuffer) + 296)));
}

void CCosmicText::DirectDrawText(const std::string &text, Point where, float fontSize, const Color &col, const Point &bounds, Alignment align)
{
    auto rect = Rectangle::FromCorner(where, bounds);
    DirectDrawText(Format(text), rect, fontSize, fontSize, col, align);
}

float CCosmicText::SimpleLineWidth(const std::string &text, float fontSize, Alignment align)
{
    CtrBuffer buf = get_cached(text, fontSize, fontSize, 4096, fontSize, align);

    return ctrBufferSingleLen(ctr, buf, 0);
}

CtrBufferDimensions CCosmicText::TextDimensions(const std::string &text, float fontSize, float lineHeight, Point maximumSize)
{
    CtrBuffer buf = get_cached(text, fontSize, lineHeight, maximumSize.X(), maximumSize.Y(), Alignment::LEFT);
    return ctrBufferDimensions(ctr, buf);
}

string CCosmicText::Format(const std::string &str)
{
    string ret{};
    ret.reserve(str.size() + 4);
    size_t codep = 0;
    wstring_convert<codecvt_utf8<char32_t>, char32_t> convert;
    bool donext = false;
    while (codep != string::npos) {
        char32_t decoded = Utf8::DecodeCodePoint(str, codep);
        if(decoded == '_')
            donext = show_underlines;
        else if(decoded == '\n')
        {
            ret.append("\u2029");
        }
        else
        {
            ret.append(convert.to_bytes(decoded));
            if(donext)
                ret.append("\u0332");
            donext = false;
        }
    }
    return ret;
}

void CCosmicText::ShowUnderlines(bool show)
{
    show_underlines = show || Preferences::Has("Always underline shortcuts");
}
