#ifndef CCOSMIC_TEXT_H_
#define CCOSMIC_TEXT_H_

#include <string>
#include <vector>

#include <ccosmic_text_gl.h>



class CCosmicText {
public:
    static void Init(const std::vector<const char *> &fonts);
    static std::string Debug();

    static CtrBuffer CreateBuffer(float fontSize, float lineHeight);

};

#endif
