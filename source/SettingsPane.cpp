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

#include "Preferences.h"
#include "Rectangle.h"
#include "RenderBuffer.h"
#include "Screen.h"
#include "shader/FillShader.h"
#include "shader/RingShader.h"
#include "text/Table.h"
#include "text/alignment.hpp"
#include "text/layout.hpp"

#include <iostream>
#include <memory>
#include <optional>

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
			section.Invalidate();
	}

	if(!buffer.has_value() || !(buffer->Width() == bounds.Width() && buffer->Height() == bounds.Height()))
		buffer.emplace(bounds.Dimensions());

	scroll.SetDisplaySize(bounds.Height());
	scrollBar.displaySizeFraction = scroll.DisplaySize() / scroll.MaxValue();
	scroll.Step();

	{
		auto target = buffer->SetTarget();
		double y = -scroll.AnimatedValue();
		for(auto &section : sections)
		{
			section.Draw(
				Rectangle::FromCorner(
					Screen::TopLeft() + Point(0, y),
					Point(bounds.Dimensions().X(), section.Height())
				)
			);
			y += section.Height() + LINE_HEIGHT;
		}
	}

	buffer->SetFadePadding(
		scroll.IsScrollAtMin() ? 0 : 20,
		scroll.IsScrollAtMax() ? 0 : 20
	);
	buffer->Draw(bounds.Center(), bounds.Dimensions());

	const float SCROLLBAR_OFFSET = 5;
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
	return ret;
}



bool SettingsPane::Hover(int x, int y)
{
	scrollBar.Hover(x, y);

	if(bounds.Contains(Point(x, y)))
	{
		hoverPoint = Point(x, y);
		return true;
	}

	return false;
}



bool SettingsPane::Scroll(double dx, double dy)
{
	if(hoverPoint)
		scroll.Scroll(-dy * Preferences::ScrollSpeed());
	return hoverPoint.has_value();
}



SettingsPane::Section::Section(unique_ptr<SectionHeader> &&header, vector<unique_ptr<Item>> &&items)
	: header(std::move(header)), items(std::move(items))
{
	buf = nullopt;
}



void SettingsPane::Section::Invalidate()
{
	buf = nullopt;
}



void SettingsPane::Section::Draw(const Rectangle &at)
{
	if(!buf)
	{
		buf.emplace(at.Dimensions());
		dirty = true;
	}

	if(dirty)
	{
		RenderBuffer::RenderTargetGuard guard = buf->SetTarget();

		Table table;
		table.AddColumn(0, Layout(Alignment::LEFT));
		table.AddColumn(at.Dimensions().X(), Layout(Alignment::RIGHT));
		table.SetUnderline(0, at.Dimensions().X());

		table.DrawAt(Screen::TopLeft());
		table.SetRowHeight(LINE_HEIGHT);

		auto rowColor = header->RowColor();
		auto textColor = header->TextColor();
		if(rowColor)
		{
			table.SetColor(*rowColor);
			table.DrawHighlight();
		}
		table.SetColor(textColor);
		table.DrawUnderline();
		table.Draw(header->Name());
		auto value = header->ItemType();
		if (value)
			table.Draw(*value);
		else
			table.Advance();

		for(const auto &it : items)
		{
			auto rowColor = it->RowColor();
			if(rowColor)
			{
				table.SetColor(*rowColor);
				table.DrawHighlight();
			}
			table.SetColor(textColor);
			table.Draw(it->Name());
			auto value = it->Value();
			table.Draw(value);
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
