#include "Aquila/UI/Widgets/Separator.h"

namespace Aquila::UI::Core {

Separator::Separator() {
	AddClass("separator");
	ApplyOrientation();
}

Separator::Separator(bool vertical) : m_Vertical(vertical) {
	AddClass("separator");
	ApplyOrientation();
}

void Separator::SetVertical(bool vertical) {
	if (vertical == m_Vertical) {
		return;
	}
	m_Vertical = vertical;
	ApplyOrientation();
}

void Separator::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "vertical") {
		SetVertical(value == "true");
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

void Separator::ApplyOrientation() {
	StyleProperties sp;
	if (m_Vertical) {
		sp.width = StyleLength::Pixel(1.f);
		sp.height = StyleLength::Grow();
		sp.minHeight = StyleLength::Pixel(0.f);
	} else {
		sp.width = StyleLength::Grow();
		sp.height = StyleLength::Pixel(1.f);
		sp.minWidth = StyleLength::Pixel(0.f);
	}
	MergeStyle(sp);
}

} // namespace Aquila::UI::Core
