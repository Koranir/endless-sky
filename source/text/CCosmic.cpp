#include "CCosmic.h"

#include "../Files.h"
#include "../Screen.h"
#include "alignment.hpp"

#include <SDL_video.h>
#include <cstdint>

using namespace std;

namespace {
    FfiCtr ctr;

    class ExceptGuard {
    public:
        ExceptGuard() {}
        void Set(FfiCtr ptr) { ctrptr = ptr; }
        ~ExceptGuard() { if(ctrptr) ctrDeinit(ctrptr); }

    private:
        FfiCtr ctrptr = nullptr;
    };
    ExceptGuard _ctrGuard;
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

CtrBuffer CCosmicText::CreateBuffer(float fontSize, float lineHeight)
{
    return ctrCreateBuffer(ctr, fontSize, lineHeight);
}

void CCosmicText::DrawText(CtrBuffer buf, const std::string &text, CtrRect where, const Color &col, Alignment align)
{
    ctrSetBufferSize(ctr, buf, where.max[0] - where.min[0], where.max[1] - where.min[1]);
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
    ctrDrawBuffer(ctr, buf, col.Rgba8(), where, Screen::RawWidth(), Screen::RawHeight());
}

void CCosmicText::DirectDrawText(const std::string &text, Rectangle where, float fontSize, float lineHeight, const Color &col, Alignment align)
{
    auto buf = ctrCreateBuffer(ctr, fontSize, lineHeight);
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

    ctrDrawBuffer(ctr, buf, col.Rgba8(), CtrRect {
        {static_cast<int32_t>(where.Left()), static_cast<int32_t>(where.Bottom())},
        {static_cast<int32_t>(where.Right()), static_cast<int32_t>(where.Top())}
    }, Screen::Width(), Screen::Height());
}
