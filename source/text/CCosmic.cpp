#include "CCosmic.h"

#include "../Files.h"

#include "ccosmic_text_gl.h"
#include <SDL_video.h>

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
