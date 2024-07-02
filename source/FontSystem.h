/* FontSystem.h
Copyright (c) 2024 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef FONT_SYSTEM_H_
#define FONT_SYSTEM_H_

#include "Color.h"
#include "Rectangle.h"
#include "text/alignment.hpp"
#include "text/truncate.hpp"

#include <cstddef>
#include <freetype/freetype.h>
#include <functional>
#include <harfbuzz/hb.h>
#include "Point.h"
#include "opengl.h"
#include <stb/stb_rect_pack.h>
#include "unicode/uscript.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>



namespace _fontSystem {
   	class CacheKey {
	public:
	    // `face` should be a `FontSystem::Face *`
		inline constexpr CacheKey(void *face, uint16_t glyph_id, float font_size) //
		    : face(face), glyph_id(glyph_id), font_size(font_size) {}

		inline constexpr bool operator==(const CacheKey &other) const noexcept { //
		    return face == other.face && glyph_id == other.glyph_id && font_size == other.font_size;
		}

	public:
		void *face;
		uint16_t glyph_id;
		float font_size;
	};
};
template<>
struct std::hash<_fontSystem::CacheKey>
{
    size_t operator()(const _fontSystem::CacheKey &key) const noexcept;
};

// Class that handles font loading, shaping, fallback, and rendering.
class FontSystem {
public:
	class Face {
	public:
		Face(const std::filesystem::path &path, int index);
		~Face();

		Face(const Face &) = delete;
		Face(Face &&) = delete;
		Face &operator=(const Face &) = delete;
		Face &operator=(Face &&) = delete;

	private:
		FT_Face ft;
		hb_font_t *hb;
		hb_face_t *hb_face;
		hb_blob_t *hb_blob;
	};

	class Font {
	public:
		// Create a new font collection.
		// This will not set any fonts, so `AddXXX()` should be called first.
		inline Font(const std::string &family_name) : family_name(family_name) {} //

		Font(const Font &) = delete;
		Font(Font &&) = delete;
		Font &operator=(const Font &) = delete;
		Font &operator=(Font &&) = delete;

		inline void AddNormal(const std::filesystem::path &path, int index) { normal.emplace(path, index); } //
		inline void AddItalic(const std::filesystem::path &path, int index) { italic.emplace(path, index); } //
		inline void AddBold(const std::filesystem::path &path, int index) { bold.emplace(path, index); } //

		// Get the italic font, and whether italicity should be faked for it.
		// If there is no specific italic font, the normal font (or bold, if it doesn't exist) will be returned.
		std::pair<Face &, bool> Italic(); //

		// Get the bold font, and whether boldness should be faked for it.
		// If there is no specific bold font, the normal font (or italic, if it doesn't exist) will be returned.
		std::pair<Face &, bool> Bold(); //

		// Get the normal font.
		// This will default in the order normal -> bold -> italic
		Face &Normal(); //

	private:
		std::string family_name;
		std::optional<Face> normal;
		std::optional<Face> italic;
		std::optional<Face> bold;
	};

	class GlyphMetrics {
	public:
		FT_Glyph_Metrics metrics;
	};

	enum class CjkResolver {
		KO,
		CN,
		JP
	};

	class MetaFace {
	public:
		MetaFace(const std::filesystem::path &path);

		std::string family_name;
		std::filesystem::path path;
		unsigned face_idx;
		bool italic;
		float weight;
	};

	enum class Style {
		BOLD,
		ITALIC,
		NORMAL,
	};

	class Attributes {
	public:
		constexpr inline Attributes(std::optional<Color> color_opt = std::nullopt, Style style = Style::NORMAL, bool underline = false) noexcept
		    : color_opt(color_opt), style(style), underline(underline) {}

	public:
		std::optional<Color> color_opt;
		Style style;
		bool underline;
	};

	using CacheKey = _fontSystem::CacheKey;

	struct ShapeGlyph {
		size_t start;
		size_t end;
		float x_advance;
		float y_advance;
		float x_offset;
		float y_offset;
		float ascent;
		float descent;
		Face *face;
		uint16_t glyph_id;
		Attributes attrs;
		bool fake_italic;
		bool fake_bold;
	};

	struct Placement {
		stbrp_rect atlas_placement;
		FontSystem::GlyphMetrics metrics;
	};

	class GlyphAtlas {
	public:
		GlyphAtlas(int width, int height);
		~GlyphAtlas();

	public:
		std::vector<stbrp_node> nodes;
		stbrp_context context;
		std::unordered_map<CacheKey, Placement> map;
		GLuint tex;
		GLuint buffer;
	};

	// Text box properties
	//
	// `font_size`: Pixel size of the em-square - the height of the font.
	// `width`: Max width of the text, will wrap if no truncation is applied.
	// `align`: If width is nullopt, then the text
	// will be to the left, middle, or right of the position, otherwise aligned
	// to the box that is the width.
	// `trunc`: If truncating, the text will be truncated over the width instead
	// of wrapped. i.e. Truncate::Left will truncate the left and leave only
	// the rightmost characters that fit in the width.
	class TextBox {
	public:
		inline TextBox(
			float font_size,
			std::optional<unsigned int> width = std::nullopt,
			Alignment align = Alignment::LEFT,
			Truncate trunc = Truncate::NONE
		) : font_size(font_size), width(width), align(align), trunc(trunc) {}

	public:
		float font_size;
		std::optional<unsigned int> width;
		Alignment align;
		Truncate trunc;
	};

public:
	FontSystem(const std::vector<MetaFace> &fontFaces);

	// Draw some text to the screen.
	//
	// `position`: Source of the drawn text, with the `y` being the baseline.
	// `box`: the text's layouting properties.
	// `text`: The text to draw.
	// `attrs`: Text modifiers - color, underline, bold, italic.
	void Draw(
		const Point &position,
		const TextBox &box,
		const std::string &text,
		const Attributes &attrs
	);

	// Draw some text spans to the screen.
	// See `Draw` for more info.
	void DrawSpans(
		const Point &position,
		const TextBox &box,
		const std::vector<std::pair<std::string, Attributes>> &spans
	);

	std::vector<ShapeGlyph> Shape(const std::vector<std::pair<std::u32string_view, Attributes>> &line);
	// Shapes a span with a font & attributes, returning the start indices of missing glyphs.
	std::vector<size_t> ShapeSpans(std::vector<ShapeGlyph> &glyphs, Font &font, std::u32string_view line, const Attributes &attrs);

	int Width(const std::string &text, const Attributes &attrs, float font_size); //
	int WidthSpans(const std::vector<std::pair<std::string, Attributes>> &line, float font_size); //

	Placement &GetPlacement(const CacheKey &key); //
	GlyphAtlas &Atlas(); //

	static std::vector<std::filesystem::path> AllDefaultSystemFonts();
	static std::vector<UScriptCode> LocaleFallbacksNeeded(const std::string &str); //

private:
	FT_Library ft_lib;
	hb_buffer_t *scratch_buffer;

	GlyphAtlas atlas;
	std::unordered_map<CacheKey, Placement> atlasMap;

	// Map of hash of (string, attrs)
	std::unordered_map<size_t, std::vector<ShapeGlyph>> glyphCache;
	std::vector<ShapeGlyph> &GetCachedGlyphs(const std::string &text, const Attributes &attrs);

	std::vector<std::unique_ptr<Font>> fonts;
	std::unordered_map<UScriptCode, Font *> script_fallback_fonts;

};


#endif
