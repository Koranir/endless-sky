/* SettingsPane.h
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

#pragma once

#include "Color.h"
#include "Command.h"
#include "Panel.h"
#include "RenderBuffer.h"
#include "ScrollBar.h"
#include "ScrollVar.h"
#include "UI.h"
#include "text/Table.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Rectangle;



class SettingsPane : public Panel
{
public:
	class Section;

	class UpdateOnDropPanel : public Panel
	{
	protected:
		~UpdateOnDropPanel();

	private:
		Section *parent = nullptr;

		friend class SettingsPane;
	};
	class Item
	{
	public:
		virtual ~Item() = 0;

		virtual std::string Name() = 0;
		virtual std::string Value() = 0;
		// Toggle the item's value, or return another panel to collect more inputs.
		virtual std::optional<UpdateOnDropPanel *> Clicked() = 0;
		virtual void ResetToDefault() = 0;

		// Get a secondary panel for additional information. May or may not be called.
		virtual Panel *NewInfoUI(UI *parent, const Rectangle &bounds) = 0;
		
		virtual std::optional<Color> RowColor() = 0;
		virtual std::optional<Color> ValueColor() = 0;
		virtual Color TextColor() = 0;
	};

	class SectionHeader
	{
	public:
		virtual ~SectionHeader() = 0;

		virtual std::string Name() = 0;
		virtual std::optional<std::string> ItemType() = 0;
		virtual std::optional<Color> RowColor() = 0;
		virtual Color TextColor() = 0;
	};

	class Section
	{
	public:
		Section(std::unique_ptr<SectionHeader> &&header, std::vector<std::unique_ptr<Item>> &&items);
		void Invalidate(bool scaleChanged = false);

		void Draw(const Rectangle &at, SettingsPane &parent, size_t baseIndex);

		std::vector<std::unique_ptr<Item>> &Items();
		std::optional<size_t> ItemForY(double y);
		int Height() const;

	private:
		std::unique_ptr<SectionHeader> header;
		std::vector<std::unique_ptr<Item>> items;

		bool dirty = true;

		std::optional<RenderBuffer> buf = std::nullopt;
	};
	friend class Section;

public:
	SettingsPane(std::vector<Section> &&sections, const Rectangle &bounds, const std::optional<Rectangle> &infoBounds);

	void UpdateScale(double scale);


	void Draw() override;
	bool Click(int x, int y, int clicks) override;
	bool Hover(int x, int y) override;
	bool Drag(double dx, double dy) override;
	bool Release(int x, int y) override;
	bool Scroll(double dx, double dy) override;
	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;

protected:
	std::pair<Section *, size_t> SectionForIndex(size_t idx);
	std::optional<std::pair<Section *, size_t>> SectionForY(double y);

	void SetInfoPanel(size_t idx);
	void TriggerItem(size_t idx);

	void EnsureOnScreen(size_t idx, int steps = 5);

protected:
	std::optional<size_t> mouseOver;
	std::optional<size_t> keyboardOver;
	std::optional<size_t> focused;

	std::vector<Section> sections;

	Rectangle bounds;
	int scale;

	std::optional<Rectangle> infoBounds;
	std::optional<Panel *> infoPanel;

	bool dragging = false;
	Point dragSource;
	std::optional<Point> hoverPoint;

	std::optional<RenderBuffer> buffer;

	ScrollVar<double> scroll;
	ScrollBar scrollBar;

};
