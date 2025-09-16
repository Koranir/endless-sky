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

class ColumnWidget : public Widget {
public:
	ColumnWidget() noexcept;

	template<typename... Widgets>
	ColumnWidget(Widgets&&... widgets) {
		(Add(make_element(widgets)),...);
	}

	virtual Size<Length> BoundSize() override;
	virtual void Layout(
		Renderer &renderer,
		Size<float> limits
	) override;
	virtual Response Update(
		Renderer &renderer,
		const Event &event,
		Rect<float> bounds,
		const Cursor &cursor,
		Clipboard &clipboard,
		std::vector<Message> &messages
	) override;
	virtual void Draw(
		Renderer &renderer,
		const DrawContext &drawContext,
		Rect<float> bounds,
		const Cursor &cursor
	) override;
	virtual MouseInteraction Interaction(
		Renderer &renderer,
		Rect<float> bounds,
		const Cursor &cursor
	) override;

public:
	ColumnWidget &&Add(Element element);

private:
	// Element, allocated length pairs.
	std::vector<std::pair<Element, float>> children;
	Size<Length> size;

};

template<typename... Widgets>
ColumnWidget Column(Widgets&&... widgets) {
	ColumnWidget ret;
	(ret.Add(make_element(widgets)),...);
	return ret;
}

}
}
