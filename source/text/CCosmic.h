#ifndef CCOSMIC_TEXT_H_
#define CCOSMIC_TEXT_H_

#include <string>
#include <vector>



class CCosmicText {
public:
    static void Init(const std::vector<const char *> &fonts);
    static std::string Debug();

};

#endif
