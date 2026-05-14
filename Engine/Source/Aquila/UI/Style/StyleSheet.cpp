#include "Aquila/UI/Style/StyleSheet.h"
#include "Aquila/UI/Core/View.h"

namespace Aquila::UI {

void StyleSheet::AddRule(StyleRule::SelectorType type, std::string selector, std::string pseudoClass,
						 StyleProperties props) {
	int32 specificity = 0;
	switch (type) {
	case StyleRule::SelectorType::Type:
		specificity = 1;
		break;
	case StyleRule::SelectorType::Class:
		specificity = 10;
		break;
	case StyleRule::SelectorType::Id:
		specificity = 100;
		break;
	}
	// Pseudo-classes add 10 to specificity (same as CSS).
	// :pressed > :hover > :focus so they stack naturally when specificity is equal.
	if (!pseudoClass.empty()) {
		specificity += 10;
	}

	m_Rules.push_back({ type, std::move(selector), std::move(pseudoClass), specificity, std::move(props) });
}

ComputedStyle StyleSheet::Resolve(const Core::View &view, const ComputedStyle *parentComputed) const {
	ComputedStyle result; // initialised with all defaults via ComputedStyle ctor

	// inherited properties from parent
	if (parentComputed != nullptr) {
		result.color = parentComputed->color;
		result.fontSize = parentComputed->fontSize;
		result.fontFamily = parentComputed->fontFamily;
	}

	// collect and sort matching rules by specificity (ascending, so highest wins last)
	std::vector<const StyleRule *> matching;
	for (const auto &rule : m_Rules) {
		if (Matches(rule, view)) {
			matching.push_back(&rule);
		}
	}
	std::ranges::stable_sort(matching, {}, [](const StyleRule *rule) { return rule->specificity; });

	// apply matching rules in specificity order
	for (const StyleRule *rule : matching) {
		ApplyProperties(result, rule->properties);
	}

	// inline style wins over everything
	ApplyProperties(result, view.GetStyle());

	return result;
}

bool StyleSheet::Matches(const StyleRule &rule, const Core::View &view) const {
	// Check selector
	bool selectorMatch = false;
	switch (rule.selectorType) {
	case StyleRule::SelectorType::Type:
		selectorMatch = (view.GetTypeName() == rule.selector);
		break;
	case StyleRule::SelectorType::Class: {
		const auto &classes = view.GetClasses();
		selectorMatch = (std::ranges::find(classes, rule.selector) != classes.end());
		break;
	}
	case StyleRule::SelectorType::Id:
		selectorMatch = (view.GetId() == rule.selector);
		break;
	}

	if (!selectorMatch) {
		return false;
	}

	// Check pseudo-class
	if (rule.pseudoClass.empty()) {
		return true;
	}
	if (rule.pseudoClass == "hover") {
		return view.IsHovered();
	}
	if (rule.pseudoClass == "pressed") {
		return view.IsPressed();
	}
	if (rule.pseudoClass == "focus") {
		return view.IsFocused();
	}

	return false;
}

void StyleSheet::ApplyProperties(ComputedStyle &out, const StyleProperties &props) const {
	if (props.backgroundColor) {
		out.backgroundColor = *props.backgroundColor;
	}
	if (props.borderColor) {
		out.borderColor = *props.borderColor;
	}
	if (props.borderWidth) {
		out.borderWidth = *props.borderWidth;
	}
	if (props.borderRadius) {
		out.borderRadius = *props.borderRadius;
	}
	if (props.opacity) {
		out.opacity = *props.opacity;
	}
	if (props.overflow) {
		out.overflow = *props.overflow;
	}
	if (props.display) {
		out.display = *props.display;
	}

	if (props.width) {
		out.width = *props.width;
	}
	if (props.height) {
		out.height = *props.height;
	}

	// Shorthands expand first, then longhands override
	if (props.min) {
		out.minWidth = *props.min;
		out.minHeight = *props.min;
	}
	if (props.max) {
		out.maxWidth = *props.max;
		out.maxHeight = *props.max;
	}
	if (props.minWidth) {
		out.minWidth = *props.minWidth;
	}
	if (props.maxWidth) {
		out.maxWidth = *props.maxWidth;
	}
	if (props.minHeight) {
		out.minHeight = *props.minHeight;
	}
	if (props.maxHeight) {
		out.maxHeight = *props.maxHeight;
	}

	if (props.padding) {
		out.padding = *props.padding;
	}
	if (props.gap) {
		out.gap = *props.gap;
	}

	if (props.flexDirection) {
		out.flexDirection = *props.flexDirection;
	}
	if (props.justifyContent) {
		out.justify = *props.justifyContent;
	}
	if (props.alignItems) {
		out.align = *props.alignItems;
	}
	if (props.flexWrap) {
		out.wrap = *props.flexWrap;
	}
	if (props.flexGrow) {
		out.flexGrow = *props.flexGrow;
	}

	if (props.position) {
		out.position = *props.position;
	}
	if (props.top) {
		out.top = *props.top;
	}
	if (props.right) {
		out.right = *props.right;
	}
	if (props.bottom) {
		out.bottom = *props.bottom;
	}
	if (props.left) {
		out.left = *props.left;
	}
	if (props.zIndex) {
		out.zIndex = *props.zIndex;
	}

	if (props.color) {
		out.color = *props.color;
	}
	if (props.fontSize) {
		out.fontSize = *props.fontSize;
	}
	if (props.fontFamily) {
		out.fontFamily = *props.fontFamily;
	}
	if (props.transitionDuration) {
		out.transitionDuration = *props.transitionDuration;
	}
	if (props.transitionEasing) {
		out.transitionEasing = *props.transitionEasing;
	}
	if (props.textAlign) {
		out.textAlign = *props.textAlign;
	}
	if (props.boxShadows) {
		out.boxShadows = *props.boxShadows;
	}
}

} // namespace Aquila::UI
