#include "Aquila/UI/Style/StyleSheet.h"
#include "Aquila/UI/Core/View.h"
#include <cmath>

namespace Aquila::UI {

bool MediaCondition::Evaluate(float w, float h) const {
	const float v = (axis == Axis::Width) ? w : h;
	switch (op) {
	case Op::Less:
		return v < value;
	case Op::LessEq:
		return v <= value;
	case Op::Greater:
		return v > value;
	case Op::GreaterEq:
		return v >= value;
	case Op::Equal:
		return std::abs(v - value) < 0.5f;
	}
	return false;
}

static bool EvaluateBlock(const MediaBlock &block, float w, float h) {
	for (const auto &cond : block.conditions) {
		if (!cond.Evaluate(w, h)) {
			return false;
		}
	}
	return true;
}

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
	if (!pseudoClass.empty()) {
		specificity += 10;
	}

	m_Rules.push_back({ type, std::move(selector), std::move(pseudoClass), specificity, std::move(props) });
}

void StyleSheet::AddMediaBlock(MediaBlock block) {
	m_MediaBlocks.push_back(std::move(block));
}

void StyleSheet::AddContainerBlock(MediaBlock block) {
	m_ContainerBlocks.push_back(std::move(block));
}

void StyleSheet::AddVariable(std::string name, std::string value) {
	m_Variables[std::move(name)] = std::move(value);
}

const std::string *StyleSheet::GetVariable(std::string_view name) const {
	const auto it = m_Variables.find(std::string(name));
	return it != m_Variables.end() ? &it->second : nullptr;
}

void StyleSheet::ApplyMatchingRules(ComputedStyle &out, const std::vector<StyleRule> &rules,
									const Core::View &view) const {
	std::vector<const StyleRule *> matching;
	for (const auto &rule : rules) {
		if (Matches(rule, view)) {
			matching.push_back(&rule);
		}
	}
	std::ranges::stable_sort(matching, {}, [](const StyleRule *r) { return r->specificity; });
	for (const StyleRule *rule : matching) {
		ApplyProperties(out, rule->properties);
	}
}

ComputedStyle StyleSheet::Resolve(const Core::View &view, const ComputedStyle *parentComputed,
								  const ResolveContext &ctx) const {
	ComputedStyle result;

	if (parentComputed) {
		result.color = parentComputed->color;
		result.fontSize = parentComputed->fontSize;
		result.fontFamily = parentComputed->fontFamily;
	}

	std::vector<const StyleRule *> matching;
	for (const auto &rule : m_Rules) {
		if (Matches(rule, view)) {
			matching.push_back(&rule);
		}
	}
	std::ranges::stable_sort(matching, {}, [](const StyleRule *r) { return r->specificity; });
	for (const StyleRule *rule : matching) {
		ApplyProperties(result, rule->properties);
	}

	for (const auto &block : m_MediaBlocks) {
		if (EvaluateBlock(block, ctx.viewportSize.x, ctx.viewportSize.y)) {
			ApplyMatchingRules(result, block.rules, view);
		}
	}

	for (const auto &block : m_ContainerBlocks) {
		if (EvaluateBlock(block, ctx.containerSize.x, ctx.containerSize.y)) {
			ApplyMatchingRules(result, block.rules, view);
		}
	}

	ApplyProperties(result, view.GetStyle());

	return result;
}

bool StyleSheet::Matches(const StyleRule &rule, const Core::View &view) const {
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
	if (rule.pseudoClass == "disabled") {
		return !view.IsEnabled();
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
	if (props.paddingLeft) {
		out.padding.left = *props.paddingLeft;
	}
	if (props.paddingRight) {
		out.padding.right = *props.paddingRight;
	}
	if (props.paddingTop) {
		out.padding.top = *props.paddingTop;
	}
	if (props.paddingBottom) {
		out.padding.bottom = *props.paddingBottom;
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
	if (props.accentColor) {
		out.accentColor = *props.accentColor;
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
