/* ControlSettingsPanel.cpp
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

#include "ControlSettingsPanel.h"

#include "Dialog.h"
#include "GameData.h"
#include "Rectangle.h"
#include "SettingsPane.h"

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <optional>
#include <utility>
#include <vector>

using namespace std;

namespace {
	static const string NAV = "Keyboard Navigation";
	static const vector<Command> NAV_COMMANDS = {
		Command::FORWARD,
		Command::LEFT,
		Command::RIGHT,
		Command::BACK,
		Command::AFTERBURNER,
		Command::AUTOSTEER,
		Command::LAND,
		Command::JUMP,
	};
	static const string FLEET = "Fleet";
	static const vector<Command> FLEET_COMMANDS = {
		Command::DEPLOY,
		Command::FIGHT,
		Command::GATHER,
		Command::HOLD,
		Command::AMMO,
		Command::HARVEST,
	};
	static const string TARGET = "Targeting";
	static const vector<Command> TARGET_COMMANDS = {
		Command::NEAREST,
		Command::TARGET,
		Command::HAIL,
		Command::BOARD,
		Command::NEAREST_ASTEROID,
		Command::SCAN,
	};
	static const string WEAPON = "Weapons";
	static const vector<Command> WEAPON_COMMANDS = {
		Command::PRIMARY,
		Command::TURRET_TRACKING,
		Command::SELECT,
		Command::SECONDARY,
		Command::CLOAK,
		Command::MOUSE_TURNING_HOLD,
	};
	static const string INTERFACE = "Interface";
	static const vector<Command> INTERFACE_COMMANDS = {
		Command::MENU,
		Command::MAP,
		Command::INFO,
		Command::FULLSCREEN,
		Command::FASTFORWARD,
		Command::HELP,
		Command::MESSAGE_LOG
	};
	static const array<pair<string, vector<Command>>, 5> COMMAND_SETTINGS = {{
		{NAV, NAV_COMMANDS},
		{FLEET, FLEET_COMMANDS},
		{TARGET, TARGET_COMMANDS},
		{WEAPON, WEAPON_COMMANDS},
		{INTERFACE, INTERFACE_COMMANDS},
	}};

	vector<SettingsPane::Section> Sections()
	{
		vector<SettingsPane::Section> sections;

		for(const auto &[name, commands] : COMMAND_SETTINGS)
		{
			unique_ptr<SettingsPane::SectionHeader> header = make_unique<ControlSettingsPanel::SectionHeader>(name);
			vector<unique_ptr<SettingsPane::Item>> items;
			for(const auto &command : commands)
				items.emplace_back(make_unique<ControlSettingsPanel::Item>(command, reinterpret_cast<ControlSettingsPanel::SectionHeader *>(header.get())));
			sections.emplace_back(std::move(header), std::move(items));
		}

		return sections;
	}
}



ControlSettingsPanel::ControlSettingsPanel(const Rectangle &bounds, const optional<Rectangle> &infoBounds)
	: SettingsPane(Sections(), bounds, infoBounds)
{
	cout << Sections().size() << "\n";
}



ControlSettingsPanel::SectionHeader::SectionHeader(const string &name)
	: name(name)
{
}



ControlSettingsPanel::SectionHeader::~SectionHeader()
{
}



string ControlSettingsPanel::SectionHeader::Name()
{
	return name;
}



optional<string> ControlSettingsPanel::SectionHeader::ItemType()
{
	return {"Key"};
}



optional<Color> ControlSettingsPanel::SectionHeader::RowColor()
{
	if(error)
		return {*GameData::Colors().Get("warning conflict")};

	return nullopt;
}



Color ControlSettingsPanel::SectionHeader::TextColor()
{
	return *GameData::Colors().Get("medium");
}



ControlSettingsPanel::Item::Item(const Command &command, ControlSettingsPanel::SectionHeader *parent)
	: command(command), parent(parent)
{
}



ControlSettingsPanel::Item::~Item()
{
}



string ControlSettingsPanel::Item::Name()
{
	return command.Description();
}



string ControlSettingsPanel::Item::Value()
{
	return command.KeyName();
}



optional<Panel *> ControlSettingsPanel::Item::Clicked()
{
	return nullopt;
}



void ControlSettingsPanel::Item::ResetToDefault()
{
	Command::SetKey(command, command.Default().value_or(0));
}



Panel *ControlSettingsPanel::Item::NewInfoUI(UI *parent, const Rectangle &bounds)
{
	return new Dialog("eh");
}



optional<Color> ControlSettingsPanel::Item::RowColor()
{
	if(!command.HasBinding())
		return {*GameData::Colors().Get("warning no command")};
	return nullopt;
}



optional<Color> ControlSettingsPanel::Item::ValueColor()
{
	if(command.HasConflict())
		return {*GameData::Colors().Get("warning conflict")};
	return nullopt;
}



Color ControlSettingsPanel::Item::TextColor()
{
	return *GameData::Colors().Get("medium");
}
