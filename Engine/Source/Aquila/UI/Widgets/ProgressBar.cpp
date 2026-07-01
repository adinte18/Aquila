#include "Aquila/UI/Widgets/ProgressBar.h"

namespace Aquila::UI::Core {

ProgressBar::ProgressBar() {
	AddClass("progress-bar");

	StyleProperties sp;
	sp.width = StyleLength::Grow();
	sp.overflow = Overflow::Hidden;
	MergeStyle(sp);

	auto fill = CreateUnique<View>();
	fill->AddClass("progress-fill");
	m_Fill = AddChild(std::move(fill));

	SetValue(0.f);
}

void ProgressBar::SetValue(float value) {
	m_Value = std::clamp(value, 0.f, 1.f);

	StyleProperties sp;
	sp.width = StyleLength::Percent(m_Value * 100.f);
	sp.height = StyleLength::Grow();
	m_Fill->MergeStyle(sp);
}

void ProgressBar::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "value") {
		SetValue(std::stof(std::string(value)));
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

} // namespace Aquila::UI::Core
