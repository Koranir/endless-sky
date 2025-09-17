/* ui/widgets/Container.h
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

#include "../../Color.h"

#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ui {
namespace widget {

namespace ContainerStyle {
	struct Default {
		std::optional<Color> background = std::nullopt;
		std::optional<Color> text = std::nullopt;
		Border border;
		Shadow shadow;
	};

	struct None {};
}

template<typename Message>
class ContainerWidget : public Widget<Message> {
public:

public:
	ContainerWidget(Element<Message> &&widget) : widget(std::move(widget)) {}

	ContainerWidget &&Width(Length width);
	ContainerWidget &&Height(Length height);
	ContainerWidget &&Padding(ui::Padding padding);
	ContainerWidget &&AlignX(Alignment horizontal);
	ContainerWidget &&AlignY(Alignment vertical);
	ContainerWidget &&Center();
	ContainerWidget &&Style(ContainerStyle::Default style);
	ContainerWidget &&Style(ContainerStyle::None style);

	virtual Size<Length> BoundSize() override;
	virtual void Layout(
		UiEngine<Message> &engine,
		Size<float> limits
	) override;
	virtual Widget<Message>::Response Update(
		UiEngine<Message> &engine,
		const Event &event,
		Rect<float> bounds,
		const Cursor &cursor,
		Clipboard &clipboard,
		std::vector<Message> &messages
	) override;
	virtual void Draw(
		UiEngine<Message> &engine,
		const DrawContext &drawContext,
		Rect<float> bounds,
		Rect<float> viewport,
		const Cursor &cursor
	) override;
	virtual Widget<Message>::MouseInteraction Interaction(
		UiEngine<Message> &engine,
		Rect<float> bounds,
		const Cursor &cursor
	) override;

private:
	Element<Message> widget;
	std::optional<Length> width;
	std::optional<Length> height;
	ui::Padding padding;
	Alignment horizontal;
	Alignment vertical;
	std::variant<ContainerStyle::Default, ContainerStyle::None> style = ContainerStyle::None{};

};

template<typename Message>
static ContainerWidget<Message> Container(std::derived_from<Widget<Message>> auto &&widget) {
	return ContainerWidget(make_element<Message>(widget));
}

}
}

#include "Container.cpp"
