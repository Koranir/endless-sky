#ifndef CCOSMIC_TEXT_H_
#define CCOSMIC_TEXT_H_

#include "alignment.hpp"
#include "../Color.h"
#include "../Rectangle.h"

#include <string>
#include <vector>

#include <ccosmic_text_gl.h>



class CCosmicText {
public:
    static void Init(const std::vector<const char *> &fonts);
    static std::string Debug();

    static void ClearCache();

    static void DirectDrawText(const std::string &text, Rectangle where, float fontSize, float lineHeight, const Color &col, Alignment align);
    static void DirectDrawText(const std::string &text, Point where, float fontSize, const Color &col, const Point &bounds = Point(8192, 8192), Alignment align = Alignment::LEFT);

    static float SimpleLineWidth(const std::string &text, float fontSize, Alignment align = Alignment::LEFT);
    static CtrBufferDimensions TextDimensions(const std::string &text, float fontSize, float lineHeight, Point maximumSize);

    static std::string Format(const std::string &str);

    static void ShowUnderlines(bool show);

};

#endif
