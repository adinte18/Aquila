#pragma once

#include "Aquila/UI/Core/View.h"

namespace Aquila::UI::Core {

class ProgressBar : public View {
  public:
	ProgressBar();

	[[nodiscard]] std::string_view GetTypeName() const override { return "ProgressBar"; }

	void SetValue(float value);
	[[nodiscard]] float GetValue() const { return m_Value; }

	void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr) override;

  private:
	float m_Value = 0.f;
	View *m_Fill = nullptr;
};

} // namespace Aquila::UI::Core
