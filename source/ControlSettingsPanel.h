/* ControlSettingsPanel.h
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

#include "SettingsPane.h"

#include <optional>

class Rectangle;



class ControlSettingsPanel : public SettingsPane {
public:
	class SectionHeader : public SettingsPane::SectionHeader
	{
	public:
		SectionHeader(const std::string &name);
		~SectionHeader() override;
	
		std::string Name() override;
		std::optional<std::string> ItemType() override;
		std::optional<Color> RowColor() override;
		Color TextColor() override;

	private:
		void SetError(bool error);

	private:
		std::string name;
		bool error = false;

		friend class Controlitem;

	};



	class Item : public SettingsPane::Item
	{
	public:
		Item(const Command &command, SectionHeader *parent);
		~Item() override;

		std::string Name() override;
		std::string Value() override;
		// Toggle the item's value, or return another panel to collect more inputs.
		std::optional<SettingsPane::UpdateOnDropPanel *> Clicked() override;
		void ResetToDefault() override;

		// Get a secondary panel for additional information. May or may not be called.
		Panel *NewInfoUI(UI *parent, const Rectangle &bounds) override;
	
		std::optional<Color> RowColor() override;
		std::optional<Color> ValueColor() override;
		Color TextColor() override;

		void SetKey(int keycode);

	private:
		Command command;
		SectionHeader *parent;

	};

public:
	ControlSettingsPanel(const Rectangle &bounds, const std::optional<Rectangle> &infoBounds);
	ControlSettingsPanel(const ControlSettingsPanel &) = delete;
	ControlSettingsPanel(ControlSettingsPanel &&) = default;

};
