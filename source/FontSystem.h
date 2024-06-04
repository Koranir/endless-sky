#ifndef ES_LAYOUT_2_H_
#define ES_LAYOUT_2_H_

#include "Color.h"
#include "Point.h"
#include "Rectangle.h"
#include <harfbuzz/hb.h>
#include <freetype/freetype.h>

#include <filesystem>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

class FontSystem {
public:
	enum CJKResolver {
		Chinese,
		Japanese,
		Korean,
	};
	class Font {
	public:
		Font(const std::filesystem::path &fontPath, int faceIndex);
		// Font(Font &&f) : ft_face(f.ft_face), hb_face(f.hb_face), face_codepoints(f.face_codepoints), data(std::move(f.data)) {
		// 	f._moved = true;
		// };
		// Font(const Font &) = delete;
		// Font &operator=(Font &&) = delete;
		// Font &operator=(const Font &) = delete;
		~Font();
		bool HasCodepoint(uint32_t codepoint) const;

		std::pair<FT_Bitmap, FT_Glyph_Metrics> Rasterize(uint32_t codepoint, float ppm);

		std::string Name() const;

	public:
		FT_Face ft_face;
		hb_face_t *hb_face;
		hb_set_t *face_codepoints;
		// hb_font_t *hb_font;
		std::vector<char> data;

		friend FontSystem;

	};

	using CacheKeyFlags = uint32_t;
	static constexpr uint32_t CACHEKEYFLAG_FAKE_ITALIC = 1;
	struct ShapeGlyph {
    size_t start;
    size_t end;
    float x_advance;
    float y_advance;
    float x_offset;
    float y_offset;
    float ascent;
    float descent;
    std::optional<float> font_monospace_em_width;
    Font *font_id;
    uint16_t glyph_id;
  	std::optional<Color> color_opt;
    size_t metadata;
    CacheKeyFlags cache_key_flags;
	};

	struct GlyphMetrics {
		Point advance;
		Point dimensions;
		Point offset;
	};

	struct AttrsList {
		std::optional<Color> colorOpt;
		size_t metadata;
		CacheKeyFlags cacheKeyFlags;
	};

	struct FontData {
		std::string family;
		std::filesystem::path path;
		unsigned face_idx;
		bool italic;
		int weight;
	};
	static constexpr int NORMAL_WEIGHT = 400;

	struct CacheKey {
		Font *font;
		uint16_t glyph;
		float font_size;
		CacheKeyFlags flags;

		bool operator==(const CacheKey &other) const {
			return (other.font == font)
			&& (other.glyph == glyph)
			&& (other.font_size == font_size)
			&& (other.flags == flags);
		};
	};

	enum Locale {
	
	};

public:
	static void Init(const std::vector<std::pair<std::string_view, std::pair<std::filesystem::path, int>>> &font_faces);
	static FontSystem &Get();
	// (locale, (path, name))
	static std::vector<std::pair<std::string_view, std::pair<std::filesystem::path, int>>> DefaultFontFaces(CJKResolver resolver);
	FontSystem(const std::vector<std::pair<std::string_view, std::pair<std::filesystem::path, int>>> &font_faces);

	Font &FallbackFor(uint32_t codepoint);
	std::vector<size_t> ShapeFallback(std::vector<ShapeGlyph> &glyphs, Font &font, std::u32string_view line, const AttrsList &attrsList, bool spanRtl);
	std::vector<ShapeGlyph> Shape(const std::vector<std::pair<std::u32string_view, AttrsList>> &line);
	std::pair<Rectangle, FT_Glyph_Metrics> ToAtlasRect(const CacheKey &key);
	Point AtlasSize() const;
	void BlitFromAtlas(Rectangle atlasPos, Rectangle screenPos, const AttrsList &attrs) const;

	// Fonts fallback in linear order.
	std::vector<std::unique_ptr<Font>> fonts;
	hb_buffer_t *scratchBuffer;

};



#endif
