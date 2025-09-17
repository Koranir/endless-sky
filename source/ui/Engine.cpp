/* ui/Engine.cpp
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

#include "Engine.h"

#include "Core.h"
#include "Event.h"
#include "Widget.h"

namespace ui {

template<typename Message>
UiEngine<Message>::UiEngine()
{
}

template<typename Message>
Rect<float> UiEngine<Message>::Bounds() const
{
	return bounds;
}

template<typename Message>
void UiEngine<Message>::SetBounds(Rect<float> newBounds)
{
	if(bounds != newBounds)
	{
		bounds = newBounds;
		dirty = true;
	}
}

template<typename Message>
void UiEngine<Message>::SetWidget(Element<Message> &&element)
{
	widget = std::move(element);
	dirty = true;
}

template<typename Message>
void UiEngine<Message>::SetCursor(Cursor newCursor)
{
	cursor = newCursor;
}

template<typename Message>
void UiEngine<Message>::Draw(Rect<float> viewport, const ui::DrawContext &context)
{
	RelayoutIfDirty();

	widget->Draw(*this, context, bounds, viewport, cursor);
}

template<typename Message>
void UiEngine<Message>::RelayoutIfDirty()
{
	if (dirty)
	{
		widget->Layout(*this,bounds.Dimensions());

		dirty = false;
	}
}

template<typename Message>
void UiEngine<Message>::MouseClick(MouseButton button)
{
	RelayoutIfDirty();
	widget->Update(*this, event::MouseClick {
		.button = button
	}, bounds, cursor, messages);
}

template<typename Message>
std::vector<Message> UiEngine<Message>::DrainMessages()
{
	return std::move(messages);
}

}
