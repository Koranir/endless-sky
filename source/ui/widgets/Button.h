#pragma once

#include "../Widget.h"
#include "Container.h"
#include <functional>

namespace ui {
namespace widget {

template<typename Message>
class ButtonWidget : public ContainerWidget<Message> {
public:


private:
	std::function<Message()> onClick;

};

template<typename Message>
static ButtonWidget<Message> Button(std::derived_from<Widget<Message>> auto &&widget) {
	return ButtonWidget(make_element<Message>(widget));
}

}
}
