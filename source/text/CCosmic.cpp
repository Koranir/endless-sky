#include "CCosmic.h"

#include "../Files.h"
#include "../Screen.h"
#include "alignment.hpp"

#include <SDL_video.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>

#include "../Messages.h"

using namespace std;

struct ShapeKey {
    ShapeKey(const string &text, const float fontSize, const float lineHeight, const float bufWidth, const float bufHeight, const Alignment align)
        : text(text), fontSize(fontSize), lineHeight(lineHeight), bufWidth(bufWidth), bufHeight(bufHeight), align(align) {}

    bool operator==(const ShapeKey &other) const
    {
        return text == other.text && fontSize == other.fontSize && lineHeight == other.lineHeight && align == other.align;
    }

    const string &text;
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

    unordered_map<ShapeKey, unique_ptr<CCBuffer>> shapedCache;
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
    shapedCache.clear();
}

void CCosmicText::DirectDrawText(const std::string &text, Rectangle where, float fontSize, float lineHeight, const Color &col, Alignment align)
{
    const auto key = ShapeKey(text, fontSize, lineHeight, where.Width(), where.Height(), align);
    const auto maybe_buf = shapedCache.find(key);

    CtrBuffer buf;

    if(maybe_buf == shapedCache.end())
    {
        shapedCache[key] = unique_ptr<CCBuffer>(new CCBuffer(ctrCreateBuffer(ctr, fontSize, lineHeight)));
        buf = shapedCache[key]->buf;
        ctrSetBufferSize(ctr, buf, where.Width(), where.Height());
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
    }
    else
        buf = maybe_buf->second->buf;

    ctrDrawBuffer(ctr, buf, col.Argb8(), CtrRect {
        {static_cast<float>(where.Left()), static_cast<float>(where.Bottom())},
        {static_cast<float>(where.Right()), static_cast<float>(where.Top())}
    }, Screen::Width(), Screen::Height());

    Messages::Add(to_string(shapedCache.size() * (sizeof(ShapeKey) + sizeof(CtrBuffer) + 296)));
}

void CCosmicText::DirectDrawText(const std::string &text, Point where, float fontSize, const Color &col, Alignment align)
{
    // auto rect = Rectangle::FromCorner(where, Point(Screen::Right() - where.X(), Screen::Top() - where.Y()));
    auto rect = Rectangle::FromCorner(where, Point(4096, fontSize));
    DirectDrawText(text, rect, fontSize, fontSize, col, align);
}

float CCosmicText::TextWidth(const std::string &text, float fontSize, Alignment align)
{
    const auto key = ShapeKey(text, fontSize, fontSize, 4096, fontSize, align);
    const auto maybe_buf = shapedCache.find(key);

    CtrBuffer buf;

    if(maybe_buf == shapedCache.end())
    {
        shapedCache[key] = unique_ptr<CCBuffer>(new CCBuffer(ctrCreateBuffer(ctr, fontSize, fontSize)));
        buf = shapedCache[key]->buf;
        ctrSetBufferSize(ctr, buf, 4096, fontSize);
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
    }
    else
        buf = maybe_buf->second->buf;

    return ctrBufferLen(ctr, buf, 0);
}
