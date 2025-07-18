/* Messages.h
Copyright (c) 2014 by Michael Zahniser

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

#include <cstdint>
#include <deque>
#include <string>
#include <utility>
#include <vector>

class Color;



// Class representing messages that should be shown to the user. The messages
// gradually fade as the game steps forward, so each one must remember the game
// step when it came into being. If a new message is added that exactly matches
// an old one, the old version is removed before the new one is added; this is
// to keep repeated messages from filling up the whole screen.
class Messages {
public:
	enum class Importance : uint_least8_t {
		Highest,
		High,
		Info,
		Daily,
		Low
	};

	class Entry {
	public:
		Entry() = default;
		Entry(int step, const std::string &message, Importance importance)
			: step(step), message(message), importance(importance) {}

		int step;
		int deathStep = -1;
		std::string message;
		Importance importance;
	};

public:
	// Add a message to the list along with its level of importance
	// When forced, the message is forcibly added to the log, but not to the list.
	static void Add(const std::string &message, Importance importance = Importance::Low, bool force = false);
	// Add a message to the log. For messages meant to be shown
	// also on the main panel, use Add instead.
	static void AddLog(const std::string &message, Importance importance = Importance::Low, bool force = false);

	// Get the messages for the given game step. Any messages that are too old
	// will be culled out, and new ones that have just been added will have
	// their "step" set to the given value.
	static const std::vector<Entry> &Get(int step, int animationDuration);
	static const std::deque<std::pair<std::string, Messages::Importance>> &GetLog();

	// Reset the messages (i.e. because a new game was loaded).
	static void Reset();

	// Get color that should be used for drawing messages of given importance.
	static const Color *GetColor(Importance importance, bool isLogPanel);
};
