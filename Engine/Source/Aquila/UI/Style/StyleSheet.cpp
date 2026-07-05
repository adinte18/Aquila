#include "Aquila/UI/Style/StyleSheet.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StylePropertyList.h"
#include <cmath>

namespace Aquila::UI {

bool MediaCondition::evaluate(float w, float h) const {
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

static bool evaluate_block(const MediaBlock &block, float w, float h) {
	for (const auto &cond : block.conditions) {
		if (!cond.evaluate(w, h)) {
			return false;
		}
	}
	return true;
}

void StyleSheet::add_rule(StyleRule::SelectorType type, std::string selector, std::string pseudo_class,
						 StyleProperties props) {
	Int32 specificity = 0;
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
	if (!pseudo_class.empty()) {
		specificity += 10;
	}

	m_rules.push_back({ type, std::move(selector), std::move(pseudo_class), specificity, std::move(props) });
}

void StyleSheet::add_media_block(MediaBlock block) {
	m_media_blocks.push_back(std::move(block));
}

void StyleSheet::add_container_block(MediaBlock block) {
	m_container_blocks.push_back(std::move(block));
}

void StyleSheet::add_variable(std::string name, std::string value) {
	m_variables[std::move(name)] = std::move(value);
}

const std::string *StyleSheet::get_variable(std::string_view name) const {
	const auto it = m_variables.find(std::string(name));
	return it != m_variables.end() ? &it->second : nullptr;
}

void StyleSheet::apply_matching_rules(ComputedStyle &out, const std::vector<StyleRule> &rules,
									const Core::View &view) const {
	std::vector<const StyleRule *> matching;
	for (const auto &rule : rules) {
		if (matches(rule, view)) {
			matching.push_back(&rule);
		}
	}
	std::ranges::stable_sort(matching, {}, [](const StyleRule *r) { return r->specificity; });
	for (const StyleRule *rule : matching) {
		apply_properties(out, rule->properties);
	}
}

ComputedStyle StyleSheet::resolve(const Core::View &view, const ComputedStyle *parent_computed,
								  const ResolveContext &ctx) const {
	ComputedStyle result;

	if (parent_computed) {
#define AQ_STYLE_PROP(css, sp, cs, layout, anim, inherit) \
	AQ_STYLE_WHEN(inherit, result.cs = parent_computed->cs;)
		AQ_STYLE_PROPERTY_LIST
#undef AQ_STYLE_PROP
	}

	std::vector<const StyleRule *> matching;
	for (const auto &rule : m_rules) {
		if (matches(rule, view)) {
			matching.push_back(&rule);
		}
	}
	std::ranges::stable_sort(matching, {}, [](const StyleRule *r) { return r->specificity; });
	for (const StyleRule *rule : matching) {
		apply_properties(result, rule->properties);
	}

	for (const auto &block : m_media_blocks) {
		if (evaluate_block(block, ctx.viewport_size.x, ctx.viewport_size.y)) {
			apply_matching_rules(result, block.rules, view);
		}
	}

	for (const auto &block : m_container_blocks) {
		if (evaluate_block(block, ctx.container_size.x, ctx.container_size.y)) {
			apply_matching_rules(result, block.rules, view);
		}
	}

	apply_properties(result, view.get_style());

	return result;
}

bool StyleSheet::matches(const StyleRule &rule, const Core::View &view) const {
	bool selector_match = false;
	switch (rule.selector_type) {
	case StyleRule::SelectorType::Type:
		selector_match = (view.get_type_name() == rule.selector);
		break;
	case StyleRule::SelectorType::Class: {
		const auto &classes = view.get_classes();
		selector_match = (std::ranges::find(classes, rule.selector) != classes.end());
		break;
	}
	case StyleRule::SelectorType::Id:
		selector_match = (view.get_id() == rule.selector);
		break;
	}

	if (!selector_match) {
		return false;
	}

	if (rule.pseudo_class.empty()) {
		return true;
	}
	if (rule.pseudo_class == "hover") {
		return view.is_hovered();
	}
	if (rule.pseudo_class == "pressed") {
		return view.is_pressed();
	}
	if (rule.pseudo_class == "focus") {
		return view.is_focused();
	}
	if (rule.pseudo_class == "disabled") {
		return !view.is_enabled();
	}

	return false;
}

void StyleSheet::apply_properties(ComputedStyle &out, const StyleProperties &props) const {
	if (props.min) {
		out.min_width = *props.min;
		out.min_height = *props.min;
	}
	if (props.max) {
		out.max_width = *props.max;
		out.max_height = *props.max;
	}

#define AQ_STYLE_PROP(css, sp, cs, layout, anim, inherit) \
	if (props.sp) {                                        \
		out.cs = *props.sp;                                \
	}
	AQ_STYLE_PROPERTY_LIST
#undef AQ_STYLE_PROP

	if (props.padding_left) {
		out.padding.left = *props.padding_left;
	}
	if (props.padding_right) {
		out.padding.right = *props.padding_right;
	}
	if (props.padding_top) {
		out.padding.top = *props.padding_top;
	}
	if (props.padding_bottom) {
		out.padding.bottom = *props.padding_bottom;
	}
}

} // namespace Aquila::UI
