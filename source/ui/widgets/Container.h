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

#include <concepts>
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

class ContainerWidget : public Widget {
public:

public:
	ContainerWidget(std::unique_ptr<Widget> &&widget) : widget(std::move(widget)) {}

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

private:
	std::unique_ptr<Widget> widget;
	std::optional<Length> width;
	std::optional<Length> height;
	ui::Padding padding;
	Alignment horizontal;
	Alignment vertical;
	std::variant<ContainerStyle::Default, ContainerStyle::None> style = ContainerStyle::None{};

};

static ContainerWidget Container(std::derived_from<Widget> auto &&widget) {
	return ContainerWidget(std::make_unique<std::remove_cvref_t<decltype(widget)>>(std::move(widget)));
}

}
}
