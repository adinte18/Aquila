#include "Aquila/UI/Style/StyleSheet.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StylePropertyList.h"
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
#define AQ_STYLE_PROP(css, sp, cs, layout, anim, inherit) \
	AQ_STYLE_WHEN(inherit, result.cs = parentComputed->cs;)
		AQ_STYLE_PROPERTY_LIST
#undef AQ_STYLE_PROP
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
	if (props.min) {
		out.minWidth = *props.min;
		out.minHeight = *props.min;
	}
	if (props.max) {
		out.maxWidth = *props.max;
		out.maxHeight = *props.max;
	}

#define AQ_STYLE_PROP(css, sp, cs, layout, anim, inherit) \
	if (props.sp) {                                        \
		out.cs = *props.sp;                                \
	}
	AQ_STYLE_PROPERTY_LIST
#undef AQ_STYLE_PROP

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
}

} // namespace Aquila::UI
