/* SettingsPane.cpp
Copyright (c) 2025 by Daniel Yoon

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "SettingsPane.h"

#include "GameData.h"
#include "text/FontSet.h"
#include "text/Font.h"
#include "Preferences.h"
#include "Rectangle.h"
#include "RenderBuffer.h"
#include "Screen.h"
#include "shader/FillShader.h"
#include "shader/RingShader.h"
#include "text/Table.h"
#include "text/alignment.hpp"
#include "text/layout.hpp"

#include <SDL_keycode.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

using namespace std;

namespace {
	constexpr static int LINE_HEIGHT = 20;
}


SettingsPane::SettingsPane(vector<Section> &&sections, const Rectangle &bounds, const optional<Rectangle> &infoBounds)
	: sections(std::move(sections)), bounds(bounds), infoBounds(infoBounds) 
{
	double y = 0;
	for(const auto &section : this->sections)
		y += section.Height() + LINE_HEIGHT;
	scroll.SetMaxValue(y);
};



void SettingsPane::Draw()
{
	if(Screen::Zoom() != this->scale)
	{
		this->scale = Screen::Zoom();
		buffer.reset();
		for(auto &section : sections)
			section.Invalidate(true);
	}

	if(!buffer.has_value() || !(buffer->Width() == bounds.Width() && buffer->Height() == bounds.Height()))
		buffer.emplace(bounds.Dimensions());

	scroll.SetDisplaySize(bounds.Height());
	scrollBar.displaySizeFraction = scroll.DisplaySize() / scroll.MaxValue();
	scroll.Step();

	{
		auto target = buffer->SetTarget();
		double y = -scroll.AnimatedValue();
		size_t baseIndex = 0;
		for(auto &section : sections)
		{
			section.Draw(
				Rectangle::FromCorner(
					Screen::TopLeft() + Point(0, y),
					Point(bounds.Dimensions().X(), section.Height())
				),
				*this,
				baseIndex
			);
			y += section.Height() + LINE_HEIGHT;
			baseIndex += section.Items().size();
		}
	}

	buffer->SetFadePadding(
		scroll.IsScrollAtMin() ? 0 : 20,
		scroll.IsScrollAtMax() ? 0 : 20
	);
	// FillShader::Fill(bounds.Center(), bounds.Dimensions(), Color(1.).Transparent(0.25));
	buffer->Draw(bounds.Center(), bounds.Dimensions());

	const float SCROLLBAR_OFFSET = -3;
	const float POINTER_OFFSET = 5;
	if(scroll.Scrollable())
	{
		Point topRight(bounds.Center() + Point(buffer->Right() + SCROLLBAR_OFFSET, buffer->Top() + POINTER_OFFSET));
		Point bottomRight(bounds.Center() + Point(buffer->Right() + SCROLLBAR_OFFSET, buffer->Bottom() - POINTER_OFFSET));

		scrollBar.SyncDraw(scroll, topRight, bottomRight);
	}
}



void SettingsPane::UpdateScale(double scale)
{
}



bool SettingsPane::Click(int x, int y, int clicks)
{
	if(scrollBar.SyncClick(scroll, x, y, clicks))
		return true;
	
	dragging = bounds.Contains(Point(x, y));
	dragSource = Point(x, y);
	return dragging;
}



bool SettingsPane::Drag(double dx, double dy)
{
	if(scrollBar.SyncDrag(scroll, dx, dy))
		return true;

	if(dragging)
	{
		scroll.Scroll(-dy, 0);
		return true;
	}
	return false;
}



bool SettingsPane::Release(int x, int y)
{
	bool ret = dragging;
	dragging = false;
	if((dragSource - Point(x, y)).Length() < 10)
	{
		if(focused)
			SectionForIndex(*focused).first->Invalidate();
		if(mouseOver)
			TriggerItem(*mouseOver);

		focused = mouseOver;
		keyboardOver = mouseOver;
	}
	return ret;
}



void SettingsPane::TriggerItem(size_t idx)
{
	auto [section, offset] = SectionForIndex(idx);
	auto p = section->Items()[offset]->Clicked();
	if(p)
	{
		(*p)->parent = section;
		GetUI()->Push(*p);
	}
}



void SettingsPane::SetInfoPanel(size_t idx)
{
	auto [section, offset] = SectionForIndex(idx);
	section->Invalidate();

	if(infoBounds)
	{
		if(infoPanel)
			RemoveChild(*infoPanel);
		auto infoUI = section->Items()[offset]->NewInfoUI(GetUI(), *infoBounds);
		infoPanel = infoUI;
		AddChild(shared_ptr<Panel>(infoUI));
	}
}



bool SettingsPane::Hover(int x, int y)
{
	scrollBar.Hover(x, y);

	if(bounds.Contains(Point(x, y)))
	{
		hoverPoint = Point(x, y);
		auto i = SectionForY(y - bounds.Top() + scroll.AnimatedValue());
		auto prev = mouseOver;
		if(i)
		{
			auto [section, offset] = *i;
			mouseOver.emplace(offset); 
			section->Invalidate();
		}
		else
			mouseOver = nullopt;

		if(prev)
			SectionForIndex(*prev).first->Invalidate();

		if(mouseOver)
			SetInfoPanel(*mouseOver);

		return true;
	}
	else
		mouseOver = nullopt;

	return false;
}



bool SettingsPane::Scroll(double dx, double dy)
{
	if(hoverPoint)
		scroll.Scroll(-dy * Preferences::ScrollSpeed());
	return hoverPoint.has_value();
}



bool SettingsPane::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_DOWN || key == SDLK_UP)
	{
		size_t len = 0;
		for(auto &section : sections)
			len += section.Items().size();

		bool instant = false;

		auto old = keyboardOver;
		if(!keyboardOver)
		{
			instant = true;
			keyboardOver.emplace(focused.value_or(key == SDLK_UP ? len - 1 : 0));
		}
		else
			if(key == SDLK_DOWN)
			{
				*keyboardOver += 1;
				if(*keyboardOver >= len)
				{
					instant = true;
					*keyboardOver = 0;
				}
			}
			else
			{
				if(*keyboardOver == 0)
				{
					instant = true;
					*keyboardOver = len;
				}
				*keyboardOver -= 1;
			}

		focused = keyboardOver;
		if(old)
			SectionForIndex(*old).first->Invalidate();
		SectionForIndex(*focused).first->Invalidate();
		if(keyboardOver)
			SetInfoPanel(*keyboardOver);

		EnsureOnScreen(*focused, instant ? 0 : 5);
		
		return true;
	}
	if(key == SDLK_RETURN)
	{
		if(keyboardOver)
			TriggerItem(*keyboardOver);
	}

	return false;
}



pair<SettingsPane::Section *, size_t> SettingsPane::SectionForIndex(size_t idx)
{
	size_t nidx = idx;
	for(auto &section : sections)
	{
		if(nidx < section.Items().size())
			return make_pair(&section, nidx);
		nidx -= section.Items().size();
	}
	throw runtime_error("Index " + to_string(idx) + " is not within any settings pane sections (size: " + to_string(nidx) + ")");
}



optional<pair<SettingsPane::Section *, size_t>> SettingsPane::SectionForY(double y)
{
	double ny = y;
	size_t sz = 0;
	for(auto &section : sections)
	{
		// Section header
		ny -= LINE_HEIGHT;
		auto i = section.ItemForY(ny);
		if(i)
			return make_pair(&section, *i + sz);
		ny -= section.Height();
		sz += section.Items().size();
	}
	return nullopt;
}



void SettingsPane::EnsureOnScreen(size_t idx, int steps)
{
	double y = 0.;
	size_t baseIndex = idx;
	for(auto &section : sections)
	{
		if(baseIndex < section.Items().size())
		{
			y += LINE_HEIGHT + baseIndex * LINE_HEIGHT;
			break;
		}
		y += section.Height() + LINE_HEIGHT;
		baseIndex -= section.Items().size();
	}

	auto min = scroll.Value() + scroll.DisplaySize() / 3;
	auto max = scroll.Value() + scroll.DisplaySize() * 2 / 3;
	if(y < min)
		scroll.Set(y - scroll.DisplaySize() / 3, steps);
	if(y > max)
		scroll.Set(y - scroll.DisplaySize() * 2 / 3, steps);
}



SettingsPane::Section::Section(unique_ptr<SectionHeader> &&header, vector<unique_ptr<Item>> &&items)
	: header(std::move(header)), items(std::move(items))
{
}



std::optional<size_t> SettingsPane::Section::ItemForY(double y)
{
	if(y < 0)
		return nullopt;

	for(size_t i = 0; i < items.size(); i++)
	{
		if(y < LINE_HEIGHT)
			return i;
		y -= LINE_HEIGHT;
	}
	return nullopt;
}



void SettingsPane::Section::Invalidate(bool scaleChanged)
{
	if(scaleChanged)
		buf.reset();
	dirty = true;
}



void SettingsPane::Section::Draw(const Rectangle &at, SettingsPane &parent, size_t baseIndex)
{
	const Font &font = FontSet::Get(14);
	const Color &back = *GameData::Colors().Get("faint");
	const Color &medium = *GameData::Colors().Get("medium");

	if(!buf)
	{
		buf.emplace(at.Dimensions());
		dirty = true;
	}

	if(dirty)
	{
		RenderBuffer::RenderTargetGuard guard = buf->SetTarget();

		const double TEXT_LEFT = 6;
		const double TEXT_RIGHT = at.Dimensions().X() - 10;

		Table table;
		table.AddColumn(TEXT_LEFT, Layout(Alignment::LEFT));
		table.AddColumn(TEXT_RIGHT, Layout(Alignment::RIGHT));
		table.SetUnderline(0, at.Dimensions().X());

		table.DrawAt(Screen::TopLeft());
		table.SetRowHeight(LINE_HEIGHT);

		auto rowColor = header->RowColor();
		auto textColor = header->TextColor();
		if(rowColor)
			table.DrawHighlight(*rowColor);
		table.DrawUnderline(medium);
		table.SetColor(textColor);
		table.Draw(header->Name());
		auto value = header->ItemType();
		if (value)
			table.Draw(*value);
		else
			table.Advance();

		auto activeIndex = parent.focused ? optional(*parent.focused - baseIndex) : nullopt;
		auto mouseIndex = parent.mouseOver ? optional(*parent.mouseOver - baseIndex) : nullopt;

		size_t i = 0;
		for(const auto &it : items)
		{
			auto value = it->Value();
			int valueWidth = font.Width(value);

			if(mouseIndex && *mouseIndex == i)
			{
				table.SetHighlight(0, at.Dimensions().X());
				table.DrawHighlight(back);
			}
			auto rowColor = it->RowColor();
			if(rowColor)
			{
				table.SetHighlight(0, at.Dimensions().X());
				table.DrawHighlight(*rowColor);
			}
			auto valueColor = it->ValueColor();
			if(valueColor)
			{
				table.SetHighlight(TEXT_RIGHT - valueWidth - 2, TEXT_RIGHT + 2);
				table.DrawHighlight(*valueColor);
			}
			if(activeIndex && *activeIndex == i)
			{
				table.SetHighlight(TEXT_RIGHT - valueWidth - 2, TEXT_RIGHT + 2);
				table.DrawHighlight(back);
			}
			auto textColor = it->TextColor();
			table.SetColor(textColor);
			table.Draw(it->Name());
			table.Draw(value);

			i++;
		}

		dirty = false;
	}

	buf->Draw(at.Center());
}



vector<unique_ptr<SettingsPane::Item>> &SettingsPane::Section::Items()
{
	return items;
}



int SettingsPane::Section::Height() const
{
	return LINE_HEIGHT * (items.size() + 1);
}



SettingsPane::SectionHeader::~SectionHeader()
{
}


SettingsPane::Item::~Item()
{
}



SettingsPane::UpdateOnDropPanel::~UpdateOnDropPanel()
{
	parent->Invalidate();
}
