#include "harfbuzz/hb.h"
#include "freetype/freetype.h"
#include <memory>
#include <optional>
#include <string>

class FontFace {
  private:
    FT_Face ft;
    hb_face_t *hb;
};

class Font {
  public:
    Font();
    void SetNormal(std::optional<std::unique_ptr<FontFace>> normal);
    
  private:
    std::string name;
    std::optional<std::unique_ptr<FontFace>> normal;
    std::optional<std::unique_ptr<FontFace>> italic;
    std::optional<std::unique_ptr<FontFace>> bold;

};

class FontFallback {
  
};

class FontSystem {
  FontSystem();
};
