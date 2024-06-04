#include "FontSystem.h"
#include "Files.h"
#include "Logger.h"

#include "Point.h"
#include "Rectangle.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <freetype/freetype.h>
#include <freetype/ftimage.h>
#include <freetype/fttypes.h>
#include <fribidi/fribidi-bidi-types.h>
#include <fribidi/fribidi-types.h>
#include <functional>
#include <harfbuzz/hb-ot.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unistd.h>
#include <unordered_map>

#include "fribidi/fribidi.h"

#include "Shader.h"
#include "opengl.h"

#define STB_RECT_PACK_IMPLEMENTATION
extern "C" {
#include "extern/stb_rect_pack.h"
}

using namespace std;

template<>
struct std::hash<FontSystem::CacheKey> {
	size_t operator()(const FontSystem::CacheKey &key) const {
		return hash<uint16_t>()(key.glyph) ^ hash<FontSystem::Font *>()(key.font) ^ hash<FontSystem::CacheKeyFlags>()(key.flags) ^ hash<float>()(key.font_size);
	}
};

namespace {
	FT_Library ftLib;
	std::optional<FontSystem> fontSystem;

	void _destroy(void *) {}

	void GetFontsRecursive(vector<filesystem::path> &fonts, const filesystem::path &dir)
	{
		if(!(filesystem::exists(dir) && filesystem::is_directory(dir)))
			return;

		// Logger::LogError("here");

		auto iter = filesystem::recursive_directory_iterator(dir);
		for(const auto &it : iter)
		{
			// Logger::LogError(it.path());
			if(it.is_regular_file())
			{
				auto ext = it.path().extension();
				// Logger::LogError(ext);
				if(ext == ".ttf"
				   || ext == ".otf"
				   || ext == ".ttc"
				   || ext == ".otc"
				   || ext == ".TTF"
				   || ext == ".OTF"
				   || ext == ".TTC"
				   || ext == ".OTC")
				{
					fonts.emplace_back(it.path());
				}
			}
		}
	}

	vector<stbrp_node> atlas_nodes;
	stbrp_context atlas_context;
	unordered_map<FontSystem::CacheKey, pair<stbrp_rect, FT_Glyph_Metrics>> atlasMap;
	GLuint atlasTex = 0;
	GLuint atlasBuffer = 0;

	void InitAtlas(int width, int height)
	{
		atlas_nodes.resize(width);
		stbrp_init_target(&atlas_context, width, height, atlas_nodes.data(), atlas_nodes.size());
		stbrp_setup_allow_out_of_mem(&atlas_context, 1);
		atlasMap.clear();

		if(atlasTex)
			glDeleteTextures(1, &atlasTex);
		if(atlasBuffer)
			glDeleteFramebuffers(1, &atlasBuffer);

		glGenFramebuffers(1, &atlasBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, atlasBuffer);
		glGenTextures(1, &atlasTex);
		glBindTexture(GL_TEXTURE_2D, atlasTex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, atlasTex, 0);

		GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, draw_buffers);

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	std::optional<pair<stbrp_rect, FT_Glyph_Metrics>> MaybeGetAtlasCoords(FontSystem::CacheKey &key) {
		// cout << key.font << " " << key.font_size << " " << key.flags << " " << key.glyph << "\n";
		auto it = atlasMap.find(key);
		if(it == atlasMap.end())
			return nullopt;

		// cout << "found\n";
		return it->second;
	}

	pair<stbrp_rect, FT_Glyph_Metrics> ToAtlas(FontSystem::CacheKey key) {
		auto maybe_coords = MaybeGetAtlasCoords(key);
		if(maybe_coords)
			return *maybe_coords;

		auto [bitmap, metrics] = key.font->Rasterize(key.glyph, key.font_size);

		cout << "Bitmap: " << bitmap.width << "x" << bitmap.rows << ";\n";
		cout << "Metrics: Size: " << metrics.width << "x" << metrics.height << ", Advance: " <<
			metrics.horiAdvance << ", Offset: " << metrics.horiBearingX << "x" << metrics.horiBearingY << ";\n";
		cout << "SMetrics: Size: " << metrics.width / 64.0 << "x" << metrics.height / 64.0 << ", Advance: " <<
			metrics.horiAdvance / 64.0 << ", Offset: " << metrics.horiBearingX / 64.0 << "x" << metrics.horiBearingY / 64.0 << ";\n";
		
		auto rect = stbrp_rect {
			key.glyph,
			static_cast<int>(bitmap.width / 3),
			static_cast<int>(bitmap.rows),
			0,
			0,
			0
		};

		if(!stbrp_pack_rects(&atlas_context, &rect, 1))
		{
			InitAtlas(atlas_context.width * 1.2, atlas_context.height * 1.2);
			// Figure out something better later.
			return {rect, metrics};
		}

		GLuint tex;
		GLuint buf;
		glGenFramebuffers(1, &buf);
		glBindFramebuffer(GL_FRAMEBUFFER, buf);
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rect.w, rect.h, 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap.buffer);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, buf);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, atlasBuffer);

		glBlitFramebuffer(0, 0, rect.w, rect.h, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		glDeleteFramebuffers(1, &buf);
		glDeleteTextures(1, &tex);

		atlasMap[key] = {rect, metrics};

		glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		return {rect, metrics};
	}

	Shader shader;
	GLuint vao;
	GLuint vbo;
	GLfloat VERTICES[] = {
		0.0, 0.0,
		0.0, 1.0,
		1.0, 0.0,
		1.0, 1.0
	};

	GLint atlasI;
	GLint screenI;
	GLint fakeItalicI;
	GLint colorMulI;
	GLint atlasTexI;
}



static const char *VERTEX = R"-(
// Glyph atlas blit shader

uniform vec4 atlas;
uniform vec4 screen;

// uniform vec4 colorMul;
uniform int fakeItalic;

// In range [0, 1]
in vec2 vert;
out vec2 texCoord;

void main() {
	texCoord = atlas.xy + vec2(atlas.z, -atlas.w) * vert;
	if(fakeItalic == 1)
		gl_Position = vec4(screen.xy + screen.zw * vert + vec2(screen.z * vert.y * 0.5, 0.0), 0, 1);
	else
		gl_Position = vec4(screen.xy + screen.zw * vert, 0, 1);
	// gl_Position = vec4(texCoord, 0, 1);
}
)-";
static const char *FRAGMENT = R"-(
// Glyph atlas blit shader

uniform vec4 colorMul;
uniform sampler2D atlasTex;

in vec2 texCoord;
out vec4 color;

void main() {
	vec4 col = texture(atlasTex, texCoord);
	color = vec4(col.rgb * colorMul.rgb, length(col.rgb) / sqrt(3) * colorMul.a);
}
)-";
void FontSystem::BlitFromAtlas(Rectangle atlasPos, Rectangle screenPos, const AttrsList &attrs) const
{
	glUseProgram(shader.Object());
	glBindVertexArray(vao);
	glBindTexture(GL_TEXTURE_2D, atlasTex);

	glUniform4f(atlasI, atlasPos.Left(), atlasPos.Bottom(), atlasPos.Width(), atlasPos.Height());
	glUniform4f(screenI, screenPos.Left(), screenPos.Top(), screenPos.Width(), screenPos.Height());

	glUniform1i(fakeItalicI, (attrs.cacheKeyFlags & FontSystem::CACHEKEYFLAG_FAKE_ITALIC) != 0);
	auto col = attrs.colorOpt.value_or(Color());
	glUniform4fv(colorMulI, 1, col.Get());
	glUniform1i(atlasTexI, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);
}




vector<FontSystem::ShapeGlyph> FontSystem::Shape(const vector<pair<u32string_view, AttrsList>> &line)
{
	static_assert(sizeof(FriBidiChar) == sizeof(char32_t), "FriBidi char is not the same size as char32_t.");
	auto glyphs = vector<ShapeGlyph>();

	for(const auto &span : line)
	{
		const auto &[baseview, attrs] = span;
		vector<FriBidiLevel> charTypes;
		charTypes.resize(baseview.length());
		vector<FriBidiParType> parTypes;
		parTypes.resize(baseview.length(), FRIBIDI_TYPE_ON);
		vector<FriBidiChar> viewBase;
		viewBase.resize(baseview.length());
		u32string_view view(reinterpret_cast<char32_t *>(viewBase.data()), viewBase.size());

		fribidi_log2vis(reinterpret_cast<const FriBidiChar *>(baseview.data()), baseview.length(), parTypes.data(), viewBase.data(), NULL, NULL, charTypes.data());

		// for(int i = 0; i < view.length(); i++)
		// {
		// 	cout << "V: " << view[i] << "P: " << parTypes[i] << "\n";
		// }

		auto glyphStart = glyphs.size();
		auto missing = ShapeFallback(glyphs, *fonts[0], view, attrs, false);
		// Logger::LogError("Missing " + to_string(missing.size()) + " glyphs.");
		for(size_t j = 1; j < fonts.size(); j++)
		{
			if(missing.empty())
				break;

			auto fb_glyphs = vector<ShapeGlyph>();
			auto fb_missing = ShapeFallback(fb_glyphs, *fonts[j], view, attrs, false);
			// Logger::LogError("Missing " + to_string(fb_missing.size()) + " glyphs.");

			for(size_t fb_i = 0; fb_i < fb_glyphs.size();)
			{
				auto start = fb_glyphs[fb_i].start;
				auto end = fb_glyphs[fb_i].end;

				if(find(missing.cbegin(), missing.cend(), start) == missing.cend()
					|| find(fb_missing.cbegin(), fb_missing.cend(), start) != fb_missing.cend())
				{
					fb_i += 1;
					continue;
				}

				size_t missing_i = 0;
				while(missing_i < missing.size()) {
					if(missing[missing_i] >= start && missing[missing_i] < end) {
						missing.erase(missing.begin() + missing_i);
					} else {
						missing_i += 1;
					}
				}

				// Find prior glyphs
				auto i = glyphStart;
				while(i < glyphs.size()) {
					if(glyphs[i].start >= start && glyphs[i].end <= end) {
						break;
					} else {
						i += 1;
					}
				}

				// Remove prior glyphs
				while(i < glyphs.size()) {
					if(glyphs[i].start >= start && glyphs[i].end <= end) {
						glyphs.erase(glyphs.begin() + i);
					} else {
						break;
					}
				}

				while(fb_i < fb_glyphs.size()) {
					if(fb_glyphs[fb_i].start >= start && fb_glyphs[fb_i].end <= end) {
						auto fb_glyph = fb_glyphs[fb_i];
						fb_glyphs.erase(fb_glyphs.begin() + fb_i);
						glyphs.insert(glyphs.begin() + i, fb_glyph);
						i += 1;
					} else {
						break;
					}
				}
			}

			// cerr << "Shaped " << fonts[j]->Name() << '\n';
		}		
	}

	return glyphs;
}



void FontSystem::Init(const vector<pair<string_view, pair<filesystem::path, int>>> &fonts_input)
{
	auto error = FT_Init_FreeType(&ftLib);
	if(error) {
		throw runtime_error(string("Failed to initialize FreeType: ") + FT_Error_String(error));
	}

	InitAtlas(1024, 1024);
	fontSystem = {FontSystem(fonts_input)};

	shader = Shader(VERTEX, FRAGMENT);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(VERTICES), VERTICES, GL_STATIC_DRAW);

	auto stride = 2 * sizeof(GLfloat);
	auto vertI = shader.Attrib("vert");
	glEnableVertexAttribArray(vertI);
	glVertexAttribPointer(vertI, 2, GL_FLOAT, GL_FALSE, stride, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	atlasI = shader.Uniform("atlas");
	screenI = shader.Uniform("screen");
	fakeItalicI = shader.Uniform("fakeItalic");
	colorMulI = shader.Uniform("colorMul");
	atlasTexI = shader.Uniform("atlasTex");
}



FontSystem &FontSystem::Get()
{
	return *fontSystem;
}



FontSystem::FontSystem(const vector<pair<string_view, pair<filesystem::path, int>>> &fonts_input) {
	for(const auto &path : fonts_input) {
		Logger::LogError(path.second.first);
		fonts.emplace_back(new Font(path.second.first, path.second.second));
	}

	scratchBuffer = hb_buffer_create();
	
}



vector<pair<string_view, pair<filesystem::path, int>>> FontSystem::DefaultFontFaces(CJKResolver resolver)
{
	//! Stolen from https://github.com/RazrFalcon/fontdb/blob/fd24f8c9d7b57f30a6f6ba560c697a7854c94858/src/lib.rs#L352

	vector<pair<string_view, pair<filesystem::path, int>>> defaultFaces;
	defaultFaces.emplace_back("en", pair{Files::Images() + "font/atkinson-hyperlegible.ttf", 0});

	vector<filesystem::path> paths;

#if defined __linux__
	GetFontsRecursive(paths, filesystem::path("/usr/share/fonts/"));
	GetFontsRecursive(paths, filesystem::path("/usr/local/share/fonts/"));

	auto homep = getenv("HOME");
	if(homep)
	{
		GetFontsRecursive(paths, filesystem::path(homep).concat(".fonts"));
		GetFontsRecursive(paths, filesystem::path(homep).concat(".local/share/fonts"));
	}
#elif defined _WIN32
	auto homep = getenv("SYSTEMROOT");
	if(homep)
		GetFontsRecursive(paths, filesystem::path(homep).concat("Fonts"));
	else
		GetFontsRecursive(paths, filesystem::path("C:\\Windows\\Fonts"));

	auto upp = getenv("USERPROFILE");
	if(upp) {
		GetFontsRecursive(paths, filesystem::path(upp).concat("AppData\\Local\\Microsoft\\Windows\\Fonts"));
		GetFontsRecursive(paths, filesystem::path(upp).concat("AppData\\Roaming\\Microsoft\\Windows\\Fonts"));
	}
#elif defined __APPLE__
	GetFontsRecursive(paths, filesystem::path("/Library/Fonts"));
	GetFontsRecursive(paths, filesystem::path("/System/Library/Fonts"));
	GetFontsRecursive(paths, filesystem::path("/System/Library/AssetsV2"));
	GetFontsRecursive(paths, filesystem::path("/Network/Library/Fonts"));
	auto homep = getenv("HOME");
	if(homep)
		GetFontsRecursive(paths, filesystem::path(homep).concat("Library/Fonts"));
#endif

	vector<FontData> fontDatas;

	// Logger::LogError("Loading " + to_string(paths.size()) + " fonts.");
	for(const auto &path : paths)
	{
		auto blob = hb_blob_create_from_file_or_fail(path.c_str());
		if(blob) {
			auto num_faces = hb_face_count(blob);
			for(unsigned idx = 0; idx < num_faces; idx++)
			{
				auto face = hb_face_create(blob, idx);

				auto sz = hb_ot_name_get_utf8(face, HB_OT_NAME_ID_FONT_FAMILY, HB_LANGUAGE_INVALID, 0, nullptr);
				if(!sz)
					continue;

				string str;
				str.resize(sz);
				sz++;
				hb_ot_name_get_utf8(face, HB_OT_NAME_ID_FONT_FAMILY, HB_LANGUAGE_INVALID, &sz, str.data());

				auto font = hb_font_create(face);

				auto weight = hb_style_get_value(font, HB_STYLE_TAG_WEIGHT);
				auto italic = hb_style_get_value(font, HB_STYLE_TAG_ITALIC);

				Logger::LogError(str + " @ Weight: " + to_string(weight) + ", @ Italic: " + to_string(italic));

				fontDatas.push_back(FontData{
					str, path, idx, italic > 0.5, static_cast<int>(weight)
				});

				hb_font_destroy(font);
				hb_face_destroy(face);
			}
		} else {
			Logger::LogError(string("Failed to parse font file ") + path.c_str());
		}
	}

	vector<pair<string_view, string_view>> locale_and_names;
#if defined __linux__
	locale_and_names = {
		{"ar", "Noto Sans Arabic"},
		{"fa", "Noto Sans Arabic"},
		{"ko", "Noto Sans CJK KR"},
		{"zh", "Noto Sans CJK SC"},
		{"ja", "Noto Sans CJK JP"}
	};
#else
#endif

	for(const auto &fontFace : fontDatas)
	{
		auto maybe_get_fallback = [&](auto name, auto locale){
				if(fontFace.family == name)
				{
					defaultFaces.emplace_back(locale, pair{fontFace.path, fontFace.face_idx});
					stringstream ss;
					ss << "Found " << locale << " fallback";
					Logger::LogError(ss.str());
					return true;
				}
				return false;
		};

		for(const auto &it : locale_and_names)
		{
			const auto &[locale, name] = it;

			if(maybe_get_fallback(name, locale))
				break;
		}
	}

	auto cjk_resolve = [&](auto n){
		auto it = find_if(defaultFaces.begin(), defaultFaces.end(), [n](const auto &it) { return it.first == n; });
		if(it != defaultFaces.end()) {
			swap(*it, defaultFaces[1]);
		}
	};
	switch(resolver)
	{
		case CJKResolver::Korean:
			cjk_resolve("ko");
			break;
		case CJKResolver::Chinese:
			cjk_resolve("zh");
			break;
		case CJKResolver::Japanese:
			cjk_resolve("ja");
			break;
	}

	return defaultFaces;
}



pair<FT_Bitmap, FT_Glyph_Metrics> FontSystem::Font::Rasterize(uint32_t codepoint, float ppu)
{
	// auto idx = FT_Get_Char_Index(ft_face, codepoint);
	auto idx = codepoint;
	FT_Set_Char_Size(ft_face, ppu * 64, 0, 0, 0);
	FT_Load_Glyph(ft_face, idx, FT_LOAD_RENDER | FT_LOAD_TARGET_LCD);
	// assert(ft_face->glyph->bitmap.pixel_mode == FT_PIXEL_MODE_GRAY);

	// FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);

	return {ft_face->glyph->bitmap, ft_face->glyph->metrics};
}



FontSystem::Font &FontSystem::FallbackFor(uint32_t codepoint)
{
	auto fallback = find_if(fonts.begin(), fonts.end(), [codepoint](const unique_ptr<FontSystem::Font> &font){
		return font->HasCodepoint(codepoint);
	});
	if(fallback == fonts.end())
		return *fonts.front();
	else
		return **fallback;
}



bool FontSystem::Font::HasCodepoint(uint32_t codepoint) const
{
	return hb_set_has(face_codepoints, codepoint);
}



FontSystem::Font::Font(const filesystem::path &filepath, int faceIndex) {
	auto file_stream = ifstream(filepath, ios::binary);
	if(!file_stream.good())
	{
		throw runtime_error(string("Failed to read font file ") + filepath.c_str() + ": " + strerror(errno));
	}

	auto length = filesystem::file_size(filepath);
	data = vector<char>(length);
	file_stream.read(data.data(), length);

	auto err = FT_New_Memory_Face(ftLib, reinterpret_cast<unsigned char *>(data.data()), data.size(), faceIndex, &ft_face);
	if(err) {
		auto e = FT_Error_String(err);
		throw runtime_error(string("FreeType failed to read font file ") + filepath.c_str() + ": " + (e ? e : "<no error message>"));
	}

	hb_face = hb_face_create(hb_blob_create(data.data(), data.size(), HB_MEMORY_MODE_READONLY, nullptr, _destroy), faceIndex);
	if(!hb_face) {
		throw runtime_error(string("HarfBuzz failed to load font file ") + filepath.c_str());
	}
	// hb_font = hb_font_create(hb_face);

	face_codepoints = hb_set_create();
	hb_face_collect_unicodes(hb_face, face_codepoints);

	Logger::LogError("Loaded font " + Name());
}



string FontSystem::Font::Name() const
{
	auto len = hb_ot_name_get_utf8(hb_face, HB_OT_NAME_ID_FULL_NAME, HB_LANGUAGE_INVALID, 0, nullptr);
	if(!len)
		return {"<unknown>"};
	len += 1;

	string res;
	res.resize(len);
	// Account for the null terminator
	hb_ot_name_get_utf8(hb_face, HB_OT_NAME_ID_FULL_NAME, HB_LANGUAGE_INVALID, &len, res.data());
	return res;
}



vector<size_t> FontSystem::ShapeFallback(std::vector<ShapeGlyph> &glyphs, Font &font, std::u32string_view line, const AttrsList &attrsList, bool spanRtl)
{
	auto hb_font = hb_font_create(font.hb_face);
	// Logger::LogError(font.Name());

	hb_buffer_clear_contents(scratchBuffer);

	int xFontScale, yFontScale;
	hb_font_get_scale(hb_font, &xFontScale, &yFontScale);

	hb_position_t ascender;
	hb_ot_metrics_get_position_with_fallback(hb_font, HB_OT_METRICS_TAG_VERTICAL_ASCENDER, &ascender);
	hb_position_t descender;
	hb_ot_metrics_get_position_with_fallback(hb_font, HB_OT_METRICS_TAG_VERTICAL_DESCENDER, &descender);

	hb_buffer_set_direction(scratchBuffer, spanRtl ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);

	hb_buffer_add_utf32(scratchBuffer, reinterpret_cast<const uint32_t *>(line.data()), line.length(), 0, line.length());

	hb_buffer_guess_segment_properties(scratchBuffer);

	if((hb_buffer_get_direction(scratchBuffer) == HB_DIRECTION_RTL) != spanRtl) {
		throw runtime_error("Buffer directino was not what was expected.");
	}

	hb_shape(hb_font, scratchBuffer, nullptr, 0);

	unsigned iLen;
	unsigned pLen;
	auto glyphInfos = hb_buffer_get_glyph_infos(scratchBuffer, &iLen);
	auto glyphPositions = hb_buffer_get_glyph_positions(scratchBuffer, &pLen);

	assert(iLen == pLen);

	vector<size_t> missing;
	glyphs.reserve(iLen);
	auto glyphStart = glyphs.size();
	for(unsigned i = 0; i < iLen; i++)
	{
		auto info = glyphInfos[i];
		auto pos = glyphPositions[i];

		auto xAdvance = static_cast<float>(pos.x_advance) / xFontScale;
		auto yAdvance = static_cast<float>(pos.y_advance) / yFontScale;
		auto xOffset = static_cast<float>(pos.x_offset) / xFontScale;
		auto yOffset = static_cast<float>(pos.y_offset) / yFontScale;

		auto startGlyph = info.cluster;

		if(!info.codepoint)
			missing.push_back(startGlyph);

		glyphs.push_back(ShapeGlyph {
			startGlyph,
			line.length(),
			xAdvance,
			yAdvance,
			xOffset,
			yOffset,
			static_cast<float>(ascender),
			static_cast<float>(descender),
			nullopt,
			&font,
			static_cast<uint16_t>(info.codepoint),
			attrsList.colorOpt,
			attrsList.metadata,
			attrsList.cacheKeyFlags,
		});
	}

	if(spanRtl)
	{
		for(unsigned i = glyphStart + 1; i < glyphs.size(); i++)
		{
			auto nextStart = glyphs[i - 1].start;
			auto nextEnd = glyphs[i - 1].end;
			auto prev = &glyphs[i];
			if(prev->start == nextStart)
				prev->end = nextEnd;
			else
				prev->end = nextStart;
		}
	}
	else
	{
		for(unsigned i = glyphs.size() - 1; i >= glyphStart + 1; i--)
		{
			auto nextStart = glyphs[i].start;
			auto nextEnd = glyphs[i].end;
			auto prev = &glyphs[i - 1];
			if(prev->start == nextStart)
				prev->end = nextEnd;
			else
				prev->end = nextStart;
		}
	}

	hb_buffer_clear_contents(scratchBuffer);

	cerr << "F: " << font.Name() << '\n';

	return missing;
}



pair<Rectangle, FT_Glyph_Metrics> FontSystem::ToAtlasRect(const FontSystem::CacheKey &key)
{
	auto [rect, metrics] = ToAtlas(key);
	// cout << rect.x << " " << rect.y << " " << rect.w << " " << rect.h << '\n';
	auto scale = Point(1.0 / atlas_context.width, 1.0 / atlas_context.height);
	// cout << rect.x * scale.X() << " " << rect.y * scale.Y() << " " << rect.w * scale.X() << " " << rect.h * scale.Y() << '\n';
	return {Rectangle::FromCorner(floor(Point(rect.x, rect.y)) * scale, floor(Point(rect.w, rect.h)) * scale), metrics};
}



Point FontSystem::AtlasSize() const {
	return Point(atlas_context.width, atlas_context.height);
}



FontSystem::Font::~Font()
{
	// Logger::LogError("Destroying font " + Name());
	hb_set_destroy(face_codepoints);
	hb_face_destroy(hb_face);
	FT_Done_Face(ft_face);
}
