#pragma once

#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleProperties.h"

namespace Aquila::UI::Core {
class View;
}

namespace Aquila::UI {

struct StyleRule {
	enum class SelectorType : uint8 { Type, Class, Id };

	SelectorType selectorType;
	std::string selector;
	int32 specificity = 0;
	StyleProperties properties;
};

class StyleSheet {
  public:
	void AddRule(StyleRule::SelectorType type, std::string selector, StyleProperties props);

	// Returns the resolved ComputedStyle for this view.
	ComputedStyle Resolve(const Core::View &view, const ComputedStyle *parentComputed) const;

  private:
	[[nodiscard]] bool Matches(const StyleRule &rule, const Core::View &view) const;

	// Applies a sparse StyleProperties onto a dense ComputedStyle.
	// Handles shorthand expansion (min/max → per-axis).
	// Longhands always override shorthands.
	void ApplyProperties(ComputedStyle &out, const StyleProperties &props) const;

	std::vector<StyleRule> m_Rules;
};

} // namespace Aquila::UI
