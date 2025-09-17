/* ui/widgets/Column.h
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

#include "../Widget.h"

#include <utility>
#include <vector>

namespace ui {
namespace widget {

template<typename Message>
class ColumnWidget : public Widget<Message> {
public:
	ColumnWidget() noexcept;

	template<typename... Widgets>
	ColumnWidget(Widgets&&... widgets) {
		(Add(make_element<Message>(widgets)),...);
	}

	virtual Size<Length> BoundSize() override;
	virtual void Layout(
		UiEngine<Message> &renderer,
		Size<float> limits
	) override;
	virtual Widget<Message>::Response Update(
		UiEngine<Message> &renderer,
		const Event &event,
		Rect<float> bounds,
		const Cursor &cursor,
		Clipboard &clipboard,
		std::vector<Message> &messages
	) override;
	virtual void Draw(
		UiEngine<Message> &renderer,
		const DrawContext &drawContext,
		Rect<float> bounds,
		Rect<float> viewport,
		const Cursor &cursor
	) override;
	virtual Widget<Message>::MouseInteraction Interaction(
		UiEngine<Message> &renderer,
		Rect<float> bounds,
		const Cursor &cursor
	) override;

public:
	ColumnWidget &&Add(Element<Message> element);

private:
	// Element, allocated length pairs.
	std::vector<std::pair<Element<Message>, float>> children;
	Size<Length> size;

};

template<typename Message, typename... Widgets>
ColumnWidget<Message> Column(Widgets&&... widgets) {
	ColumnWidget<Message> ret;
	(ret.Add(make_element<Message>(widgets)),...);
	return ret;
}

}
}

#include "Column.cpp"
