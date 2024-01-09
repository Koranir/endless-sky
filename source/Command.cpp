/* Command.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Command.h"

#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "text/Format.h"
#include "Controller.h"

#include <SDL2/SDL.h>

#include <SDL_error.h>
#include <SDL_gamecontroller.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_stdinc.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <utility>

using namespace std;

namespace {
	// These lookup tables make it possible to map a command to its description,
	// the name of the action it is mapped to, or the action it is mapped to.
	map<Command, string> description;
	map<Command, optional<string>> actionName;
	map<Command::Action, Command> commandForAction;
	map<Command, Command::Action> actionForCommand;
	// Keep track of any actions that are mapped to multiple commands, in order
	// to display a warning to the player.
	map<Command::Action, int> actionCount;
	// Need a uint64_t 1 to generate Commands.
	const uint64_t ONE = 1;

	template<class... Ts>
	struct match : Ts... { using Ts::operator()...; };
	// explicit deduction guide
	template<class... Ts>
	match(Ts...) -> match<Ts...>;

	Command::Action LoadAction(const DataNode &node) {
		auto token = node.Token(1);

		auto first = token.front();
		if(first == 'm') {
			auto val = token.substr(1);
			return {Command::ActionKind::MouseButton{DataNode::Value(val)}};
		}
		if(first == 'u') {
			return {Command::ActionKind::None()};
		}
		if(first == 'a') {
			auto val = token.substr(2);
			return {Command::ActionKind::ControllerAxis{
				make_pair(
					static_cast<SDL_GameControllerAxis>(DataNode::Value(val)), 
					token.at(1) == '-'
				)
			}};
		}
		if(first == 'c') {
			auto val = token.substr(1);
			return {Command::ActionKind::ControllerButton{static_cast<SDL_GameControllerButton>(DataNode::Value(val))}};
		}

		return {Command::ActionKind::Keyboard{node.Value(1)}};
	}

	optional<string> GetActionName(const Command::Action &action) {
		return visit(match{
			[](const Command::ActionKind::Keyboard &key){
				return make_optional(string(SDL_GetKeyName(key.get())));
			},
			[](const Command::ActionKind::MouseButton &button){
				return make_optional(string("Mouse " + to_string(button.get())));
			},
			[](const Command::ActionKind::ControllerButton &button){
				return make_optional(string(
					SDL_GameControllerGetStringForButton(button.get())
				));
			},
			[](const Command::ActionKind::ControllerAxis &axis){
				return make_optional(string((axis.get().second ? "-" : "+") +
					string(SDL_GameControllerGetStringForAxis(axis.get().first))
				));
			},
			[](const Command::ActionKind::None &){
				return optional<string>();
			}
		}, action);
	}

	static const string UNBOUND_KEY = string("(none)");
}



// Command enumeration, including the descriptive strings that are used for the
// commands both in the preferences panel and in the saved key settings.
const Command Command::NONE(0, "");
const Command Command::MENU(ONE << 0, "Show main menu");
const Command Command::FORWARD(ONE << 1, "Forward thrust");
const Command Command::LEFT(ONE << 2, "Turn left");
const Command Command::RIGHT(ONE << 3, "Turn right");
const Command Command::BACK(ONE << 4, "Reverse");
const Command Command::MOUSE_TURNING_HOLD(ONE << 5, "Mouse controls (hold)");
const Command Command::PRIMARY(ONE << 6, "Fire primary weapon");
const Command Command::SECONDARY(ONE << 7, "Fire secondary weapon");
const Command Command::SELECT(ONE << 8, "Select secondary weapon");
const Command Command::LAND(ONE << 9, "Land on planet / station");
const Command Command::BOARD(ONE << 10, "Board selected ship");
const Command Command::HAIL(ONE << 11, "Talk to selected ship");
const Command Command::SCAN(ONE << 12, "Scan selected ship");
const Command Command::JUMP(ONE << 13, "Initiate hyperspace jump");
const Command Command::FLEET_JUMP(ONE << 14, "");
const Command Command::TARGET(ONE << 15, "Select next ship");
const Command Command::NEAREST(ONE << 16, "Select nearest hostile ship");
const Command Command::NEAREST_ASTEROID(ONE << 17, "Select nearest asteroid");
const Command Command::DEPLOY(ONE << 18, "Deploy / recall fighters");
const Command Command::AFTERBURNER(ONE << 19, "Fire afterburner");
const Command Command::CLOAK(ONE << 20, "Toggle cloaking device");
const Command Command::MAP(ONE << 21, "View star map");
const Command Command::INFO(ONE << 22, "View player info");
const Command Command::FULLSCREEN(ONE << 23, "Toggle fullscreen");
const Command Command::FASTFORWARD(ONE << 24, "Toggle fast-forward");
const Command Command::HELP(ONE << 25, "Show help");
const Command Command::FIGHT(ONE << 26, "Fleet: Fight my target");
const Command Command::GATHER(ONE << 27, "Fleet: Gather around me");
const Command Command::HOLD(ONE << 28, "Fleet: Hold position");
const Command Command::HARVEST(ONE << 29, "Fleet: Harvest flotsam");
const Command Command::AMMO(ONE << 30, "Fleet: Toggle ammo usage");
const Command Command::AUTOSTEER(ONE << 31, "Auto steer");
const Command Command::WAIT(ONE << 32, "");
const Command Command::STOP(ONE << 33, "");
const Command Command::SHIFT(ONE << 34, "");



// In the given text, replace any instances of command names (in angle brackets)
// with key names (in quotes).
string Command::ReplaceNamesWithKeys(const string &text)
{
	map<string, string> subs;
	for(const auto &it : description)
		subs['<' + it.second + '>'] = '"' + actionName[it.first].value_or(UNBOUND_KEY) + '"';

	return Format::Replace(text, subs);
}



// Create a command representing whatever is mapped to the given key code.
Command::Command(Action action)
{
	auto it = commandForAction.find(action);
	if(it != commandForAction.end())
		*this = it->second;
}



// Read the current keyboard state.
void Command::InputCommands(bool enableMouse)
{
	Clear();
	const Uint8 *keyDown = SDL_GetKeyboardState(nullptr);
	const uint32_t mouseMask = SDL_GetMouseState(nullptr, nullptr);

	auto IsActionActive = [&](const Command::Action &action){
		return visit(match{
			[&](Command::ActionKind::Keyboard code){
				return static_cast<bool>(keyDown[SDL_GetScancodeFromKey(code.get())]);
			},
			[&](Command::ActionKind::MouseButton button){
				if(enableMouse || (button.get() != SDL_BUTTON_LEFT && button.get() != SDL_BUTTON_RIGHT))
					return static_cast<bool>(SDL_BUTTON(button.get()) & mouseMask);
				return false;
			},
			[&](Command::ActionKind::ControllerButton button){
				if(Controller::Get()) {
					auto val =  static_cast<bool>(SDL_GameControllerGetButton(Controller::Get().value(), button.get()));
					cerr << SDL_GetError() << "\n";
					return val;
				}
				return false;
			},
			[](Command::ActionKind::ControllerAxis axis){
				if(Controller::Get()) {
					auto val = SDL_GameControllerGetAxis(Controller::Get().value(), axis.get().first)
						/ static_cast<double>(INT16_MAX);

					return ((axis.get().second ? 1 : -1) * val) > 0. && Controller::IsValidVal(val);
				}
				return false;
			},
			[](Command::ActionKind::None){
				return false;
			}
		}, action);
	};

	// Each command can only have one keycode, but misconfigured settings can
	// temporarily cause one keycode to be used for two commands. Also, more
	// than one key can be held down at once.
	for(const auto &it : actionForCommand)
		if(IsActionActive(it.second))
			*this |= it.first;

	// Check whether the `Shift` modifier key was pressed for this step.
	if(SDL_GetModState() & KMOD_SHIFT)
		*this |= SHIFT;
}



// Load the keyboard preferences.
void Command::LoadSettings(const string &path)
{
	DataFile file(path);

	// Create a map of command names to Command objects in the enumeration above.
	map<string, Command> commands;
	for(const auto &it : description)
		commands[it.second] = it.first;

	// Each command can only have one keycode, one keycode can be assigned
	// to multiple commands.
	for(const DataNode &node : file)
	{
		auto it = commands.find(node.Token(0));
		if(it != commands.end() && node.Size() >= 2)
		{
			Command command = it->second;
			auto loaded = LoadAction(node);
			actionForCommand[command] = loaded;
			actionName[command] = GetActionName(loaded);
		}
	}

	// Regenerate the lookup tables.
	commandForAction.clear();
	actionCount.clear();
	for(const auto &it : actionForCommand)
	{
		commandForAction[it.second] = it.first;
		++actionCount[it.second];
	}
}



// Save the keyboard preferences.
void Command::SaveSettings(const string &path)
{
	DataWriter out(path);

	for(const auto &it : actionForCommand)
	{
		auto dit = description.find(it.first);
		if(dit != description.end())
			out.Write(dit->second, it.second);
	}
}



// Set the key that is mapped to the given command.
void Command::SetAction(Command command, Action action)
{
	// Always reset *all* the mappings when one is set. That way, if two commands
	// are mapped to the same key and you change one of them, the other stays mapped.
	actionForCommand[command] = action;
	actionName[command] = GetActionName(action);

	commandForAction.clear();
	actionCount.clear();

	for(const auto &it : actionForCommand)
	{
		commandForAction[it.second] = it.first;
		++actionCount[it.second];
	}
}



// Get the description of this command. If this command is a combination of more
// than one command, an empty string is returned.
const string &Command::Description() const
{
	static const string empty;
	auto it = description.find(*this);
	return (it == description.end() ? empty : it->second);
}



// Get the name of the key that is mapped to this command. If this command is
// a combination of more than one command, an empty string is returned.
const string &Command::ActionName() const
{
	auto it = actionName.find(*this);

	return (!HasBinding() ? UNBOUND_KEY : it->second.value());
}



// Check if the key has no binding.
bool Command::HasBinding() const
{
	auto it = actionName.find(*this);

	if(it == actionName.end())
		return false;
	return it->second.has_value();
}



// Check whether this is the only command mapped to the key it is mapped to.
bool Command::HasConflict() const
{
	auto it = actionForCommand.find(*this);
	if(it == actionForCommand.end())
		return false;

	auto cit = actionCount.find(it->second);
	return (cit != actionCount.end() && cit->second > 1);
}



// Load this command from an input file (for testing or scripted missions).
void Command::Load(const DataNode &node)
{
	for(int i = 1; i < node.Size(); ++i)
	{
		static const map<string, Command> lookup = {
			{"none", Command::NONE},
			{"menu", Command::MENU},
			{"forward", Command::FORWARD},
			{"left", Command::LEFT},
			{"right", Command::RIGHT},
			{"back", Command::BACK},
			{"primary", Command::PRIMARY},
			{"secondary", Command::SECONDARY},
			{"select", Command::SELECT},
			{"land", Command::LAND},
			{"board", Command::BOARD},
			{"hail", Command::HAIL},
			{"scan", Command::SCAN},
			{"jump", Command::JUMP},
			{"mouseturninghold", Command::MOUSE_TURNING_HOLD},
			{"fleet jump", Command::FLEET_JUMP},
			{"target", Command::TARGET},
			{"nearest", Command::NEAREST},
			{"deploy", Command::DEPLOY},
			{"afterburner", Command::AFTERBURNER},
			{"cloak", Command::CLOAK},
			{"map", Command::MAP},
			{"info", Command::INFO},
			{"fullscreen", Command::FULLSCREEN},
			{"fastforward", Command::FASTFORWARD},
			{"fight", Command::FIGHT},
			{"gather", Command::GATHER},
			{"hold", Command::HOLD},
			{"ammo", Command::AMMO},
			{"nearest asteroid", Command::NEAREST_ASTEROID},
			{"wait", Command::WAIT},
			{"stop", Command::STOP},
			{"shift", Command::SHIFT}
		};

		auto it = lookup.find(node.Token(i));
		if(it != lookup.end())
			Set(it->second);
		else
			node.PrintTrace("Warning: Skipping unrecognized command \"" + node.Token(i) + "\":");
	}
}



// Reset this to an empty command.
void Command::Clear()
{
	*this = Command();
}



// Clear any commands that are set in the given command.
void Command::Clear(Command command)
{
	state &= ~command.state;
}



// Set any commands that are set in the given command.
void Command::Set(Command command)
{
	state |= command.state;
}



// Check if any of the given command's bits that are set, are also set here.
bool Command::Has(Command command) const
{
	return (state & command.state);
}



// Get the commands that are set in this and in the given command.
Command Command::And(Command command) const
{
	return Command(state & command.state);
}



// Get the commands that are set in this and not in the given command.
Command Command::AndNot(Command command) const
{
	return Command(state & ~command.state);
}



// Set the turn direction and amount to a value between -1 and 1.
void Command::SetTurn(double amount)
{
	turn = max(-1., min(1., amount));
}



// Get the turn amount.
double Command::Turn() const
{
	return turn;
}



// Check if any bits are set in this command (including a nonzero turn).
Command::operator bool() const
{
	return !!*this;
}



// Check whether this command is entirely empty.
bool Command::operator!() const
{
	return !state && !turn;
}



// For sorting commands (e.g. so a command can be the key in a map):
bool Command::operator<(const Command &command) const
{
	return (state < command.state);
}



// Get the commands that are set in either of these commands.
Command Command::operator|(const Command &command) const
{
	Command result = *this;
	result |= command;
	return result;
}



// Combine everything in the given command with this command. If the given
// command has a nonzero turn set, it overrides this command's turn value.
Command &Command::operator|=(const Command &command)
{
	state |= command.state;
	if(command.turn)
		turn = command.turn;
	return *this;
}



// Private constructor.
Command::Command(uint64_t state)
	: state(state)
{
}



// Private constructor that also stores the given description in the lookup
// table. (This is used for the enumeration at the top of this file.)
Command::Command(uint64_t state, const string &text)
	: state(state)
{
	if(!text.empty())
		description[*this] = text;
}



template<>
void DataWriter::WriteToken(const Command::Action &action)
{
	string val = visit(match{
		[](const Command::ActionKind::None &){
			return string("unassigned");
		},
		[](const Command::ActionKind::Keyboard &key)
		{
			return to_string(key.get());
		},
		[](const Command::ActionKind::MouseButton &button)
		{
			return string("m" + to_string(button.get()));
		},
		[](const Command::ActionKind::ControllerButton &button)
		{
			return string("c" + to_string(button.get()));
		},
		[](const Command::ActionKind::ControllerAxis &axis)
		{
			return string("a" + string(axis.get().second ? "-" : "+") + to_string(axis.get().first));
		}
	}, action);

	out << *before << val;
	before = &space;
}
