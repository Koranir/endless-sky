/* WrappedText.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "WrappedText.h"

#include "DisplayText.h"
#include "Font.h"

#include <cstring>
#include <optional>

using namespace std;



WrappedText::WrappedText(const Font &font)
{
	SetFont(font);
}



void WrappedText::SetAlignment(Alignment align)
{
	alignment = align;
}



// Set the truncate mode.
void WrappedText::SetTruncate(Truncate trunc)
{
	truncate = trunc;
}



// Set the wrap width. This does not include any margins.
int WrappedText::WrapWidth() const
{
	return wrapWidth;
}



void WrappedText::SetWrapWidth(int width)
{
	wrapWidth = width;
}



// Set the font to use. This will also set sensible defaults for the tab
// width, line height, and paragraph break. You must specify the wrap width
// and the alignment separately.
void WrappedText::SetFont(const Font &font)
{
	this->font = &font;

	space = font.Space();
	SetTabWidth(4 * space);
	SetLineHeight(font.Height() * 120 / 100);
	SetParagraphBreak(font.Height() * 40 / 100);
}



// Set the width in pixels of a single '\t' character.
int WrappedText::TabWidth() const
{
	return tabWidth;
}



void WrappedText::SetTabWidth(int width)
{
	tabWidth = width;
}



// Set the height in pixels of one line of text within a paragraph.
int WrappedText::LineHeight() const
{
	return lineHeight;
}



void WrappedText::SetLineHeight(int height)
{
	lineHeight = height;
}



// Set the extra spacing in pixels to be added in between paragraphs.
int WrappedText::ParagraphBreak() const
{
	return paragraphBreak;
}



void WrappedText::SetParagraphBreak(int height)
{
	paragraphBreak = height;
}



// Get the word positions when wrapping the given text. The coordinates
// always begin at (0, 0).
void WrappedText::Wrap(const string &str)
{
	SetText(str.data(), str.length());

	Wrap();
}



void WrappedText::Wrap(const char *str)
{
	SetText(str, strlen(str));

	Wrap();
}



// Get the height of the wrapped text.
int WrappedText::Height() const
{
	return height;
}



// Return the width of the longest line of the wrapped text.
int WrappedText::LongestLineWidth() const
{
	return longestLineWidth;
}



// Draw the text.
void WrappedText::Draw(const Point &topLeft, const Color &color) const
{
	if(words.empty())
		return;

	if(truncate == Truncate::NONE)
		for(const Word &w : words)
			font->Draw(w.text, w.Pos() + topLeft, color);
	else
	{
		// Currently, we only apply truncation to a line if it contains a single word.
		int h = words[0].y - 1;
		for(size_t i = 0; i < words.size(); ++i)
		{
			const Word &w = words[i];
			if(h == w.y && (i != words.size() - 1 && w.y == words[i + 1].y))
				font->Draw(w.text, w.Pos() + topLeft, color);
			else
				font->Draw({w.text, {wrapWidth, truncate}}, w.Pos() + topLeft, color);
			h = w.y;
		}
	}
}



Point WrappedText::Word::Pos() const
{
	return Point(x, y);
}



void WrappedText::SetText(const char *it, size_t length)
{
	// Clear any previous word-wrapping data. It becomes invalid as soon as the
	// underlying text buffer changes.
	words.clear();

	// Reallocate that buffer.
	text.assign(it, length);
}



// Binary search the text by width
pair<string_view, string_view> splitByWidth(const string_view word, const Font *font, const int wrapWidth, const size_t max, const size_t min, const size_t last) {
	auto width = font->Width(word.substr(0, last));

	if(last == max || last == min)
	{
		if(width > wrapWidth)
			return make_pair(word.substr(0, last - 1), word.substr(last - 1));
		else
			return make_pair(word.substr(0, last), word.substr(last));
	}
	if(width < wrapWidth)
		return splitByWidth(word, font, wrapWidth, max, last, (max + last) / 2);
	else
		return splitByWidth(word, font, wrapWidth, last, min, (min + last) / 2);
};



// Loop through all the characters in the set text, accumulating continuous ASCII characters into words.
void WrappedText::Wrap()
{
	height = 0;
	longestLineWidth = 0;

	if(text.empty() || !font)
		return;


	optional<decltype(text.begin())> wordStart = nullopt;
	size_t lineStart = 0;
	int lineWidth;
	int cursorX = 0;
	int cursorY = 0;

	auto putWord = [&lineWidth, &lineStart, &cursorX, &cursorY, this](auto word, bool isEnd){
		auto width = font->Width(word);

		if(wrapWidth)
			while(width > wrapWidth)
			{
				auto [get, rem] = splitByWidth(word, font, wrapWidth - cursorX, word.length(), 0, word.length() / 2);
				words.emplace_back(get, cursorX, cursorY);
				lineWidth += font->Width(get) + space * 2;
				AdjustLine(lineStart, lineWidth, false);

				cursorX = 0;
				cursorY += lineHeight;

				word = rem;
				width = font->Width(word);
			}
		
		if(width + cursorX > wrapWidth)
		{
			lineWidth = cursorX;
			cursorX = 0;
			cursorY += lineHeight;
			AdjustLine(lineStart, lineWidth, isEnd);
		}
		words.emplace_back(word, cursorX, cursorY);
		cursorX += width;

		lineWidth = cursorX;
	};
	
	for(auto chiter = text.begin(); chiter != text.end(); chiter++)
	{
		const char ch = *chiter;

		if(wordStart)
		{
			// Continue if the character is not whitespace.
			if(ch > ' ')
				continue;

			auto word = string_view((*wordStart).operator->(), chiter - *wordStart);
			putWord(word, false);

			wordStart = nullopt;
		}
		else if(ch > ' ')
		{
			wordStart.emplace(chiter);
			continue;
		}

		if(ch == '\n')
		{
			cursorX = 0;
			cursorY += lineHeight + paragraphBreak;
			wordStart = nullopt;
		}
		else if(ch <= ' ')
		{
			cursorX += Space(ch);
			wordStart = nullopt;
		}
	}

	if(wordStart)
	{
		auto word = string_view(wordStart.value().base());
		putWord(word, true);
	}

	height = cursorY;
}



void WrappedText::AdjustLine(size_t &lineBegin, int &lineWidth, bool isEnd)
{
	int wordCount = static_cast<int>(words.size() - lineBegin);
	int extraSpace = wrapWidth - lineWidth;

	if(lineWidth > longestLineWidth)
		longestLineWidth = lineWidth;

	// Figure out how much space is left over. Depending on the alignment, we
	// will add that space to the left, to the right, to both sides, or to the
	// space in between the words. Exception: the last line of a "justified"
	// paragraph is left aligned, not justified.
	if(alignment == Alignment::JUSTIFIED && !isEnd && wordCount > 1)
	{
		for(int i = 0; i < wordCount; ++i)
			words[lineBegin + i].x += extraSpace * i / (wordCount - 1);
	}
	else if(alignment == Alignment::CENTER || alignment == Alignment::RIGHT)
	{
		int shift = (alignment == Alignment::CENTER) ? extraSpace / 2 : extraSpace;
		for(int i = 0; i < wordCount; ++i)
			words[lineBegin + i].x += shift;
	}

	lineBegin = words.size();
	lineWidth = 0;
}



int WrappedText::Space(char c) const
{
	return (c == ' ') ? space : (c == '\t') ? tabWidth : 0;
}
