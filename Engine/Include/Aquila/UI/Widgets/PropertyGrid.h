#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Label.h"

namespace Aquila::UI::Core {

class PropertyGrid : public View {
  public:
	explicit PropertyGrid(float labelWidth = 100.f);

	[[nodiscard]] std::string_view GetTypeName() const override { return "PropertyGrid"; }

	void SetLabelWidth(float w) { m_LabelWidth = w; }
	[[nodiscard]] float GetLabelWidth() const { return m_LabelWidth; }

	View *AddRow(std::string label, Unique<View> widget);

	void AddSeparator();

  private:
	float m_LabelWidth;
};

} // namespace Aquila::UI::Core
