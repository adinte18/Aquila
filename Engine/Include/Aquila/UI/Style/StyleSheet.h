#pragma once

#include "Aquila/UI/Style/ComputedStyle.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Aquila::UI::Core {
class View;
}

namespace Aquila::UI {

struct StyleRule {
	enum class SelectorType : uint8 { Type, Class, Id };

	SelectorType selectorType;
	std::string selector;
	std::string pseudoClass;
	int32 specificity = 0;
	StyleProperties properties;
};

struct MediaCondition {
	enum class Axis { Width, Height };
	enum class Op { Less, LessEq, Greater, GreaterEq, Equal };

	Axis axis = Axis::Width;
	Op op = Op::Less;
	float value = 0.f;

	[[nodiscard]] bool Evaluate(float w, float h) const;
};

struct MediaBlock {
	std::vector<MediaCondition> conditions;
	std::vector<StyleRule> rules;
};

struct StyleResolveContext {
	vec2 viewportSize = {};
	vec2 containerSize = {};
};

class StyleSheet {
  public:
	using ResolveContext = StyleResolveContext;

	void AddRule(StyleRule::SelectorType type, std::string selector, std::string pseudoClass, StyleProperties props);

	void AddMediaBlock(MediaBlock block);
	void AddContainerBlock(MediaBlock block);

	void AddVariable(std::string name, std::string value);
	[[nodiscard]] const std::string *GetVariable(std::string_view name) const;

	[[nodiscard]] const std::vector<StyleRule> &GetRules() const { return m_Rules; }
	[[nodiscard]] const std::vector<MediaBlock> &GetMediaBlocks() const { return m_MediaBlocks; }
	[[nodiscard]] const std::vector<MediaBlock> &GetContainerBlocks() const { return m_ContainerBlocks; }

	[[nodiscard]] bool HasMediaBlocks() const { return !m_MediaBlocks.empty(); }
	[[nodiscard]] bool HasContainerBlocks() const { return !m_ContainerBlocks.empty(); }

	[[nodiscard]] ComputedStyle Resolve(const Core::View &view, const ComputedStyle *parentComputed,
										const ResolveContext &ctx = {}) const;

	[[nodiscard]] usize GetRuleCount() const { return m_Rules.size(); }

  private:
	[[nodiscard]] bool Matches(const StyleRule &rule, const Core::View &view) const;

	void ApplyProperties(ComputedStyle &out, const StyleProperties &props) const;
	void ApplyMatchingRules(ComputedStyle &out, const std::vector<StyleRule> &rules, const Core::View &view) const;

	std::vector<StyleRule> m_Rules;
	std::vector<MediaBlock> m_MediaBlocks;
	std::vector<MediaBlock> m_ContainerBlocks;
	std::unordered_map<std::string, std::string> m_Variables;
};

} // namespace Aquila::UI
