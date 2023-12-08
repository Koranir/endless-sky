#ifndef ES_TEXT_FONT_2_H_
#define ES_TEXT_FONT_2_H_

#include "ccosmic_text.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Font2 {

class FontSystem {
public:
    FontSystem(const std::vector<std::string> &fontPaths) {
        std::vector<ctSource> fontSources;
        for(const auto &str : fontPaths) {
            fontSources.emplace_back(
                ctSource_Tag_FILE_PATH,
                ctSourceData{.file = ctSlice{
                    .ptr = reinterpret_cast<const uint8_t *>(str.data()),
                    .len = str.length()
                }});
        } 
        fs = ctFontSystem_NewWithFonts(ctSources{.ptr = fontSources.data(), .len = fontSources.size()});
    };
    ~FontSystem() {
        ctFontSystem_Drop(fs);
    }

    operator ctFontSystem() {
        return fs;
    }

private:
    ctFontSystem fs;
};



class SwashCache {
public:
    SwashCache() {
        cache = ctSwashCache_New();
    };

    operator ctSwashCache() {
        return cache;
    }

private:
    ctSwashCache cache;
};



class Buffer {
public:
    Buffer(FontSystem &fs, ctMetrics metrics) : fs(fs) {
        buf = ctBuffer_New(fs, metrics);
    }
    ~Buffer() {
        ctBuffer_Drop(buf);
    }

    operator ctBuffer() {
        return buf;
    }

private:
    ctBuffer buf;
    FontSystem &fs;
};

}


#endif
