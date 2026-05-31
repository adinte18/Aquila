#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Style/StyleSheet.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include <cctype>
#include <unordered_map>

namespace Aquila::UI {

// ── Static helpers ────────────────────────────────────────────────────────────

// Finds the '}' that closes the '{' at openPos, counting nested braces.
static size_t FindMatchingClose(const std::string &src, size_t openPos) {
	int depth = 1;
	size_t i = openPos + 1;
	while (i < src.size() && depth > 0) {
		if (src[i] == '{') ++depth;
		else if (src[i] == '}') --depth;
		++i;
	}
	return depth == 0 ? i - 1 : std::string::npos;
}

// First pass: collect every --name: value declaration from anywhere in the text.
static std::unordered_map<std::string, std::string> ExtractVariables(std::string_view src) {
	std::unordered_map<std::string, std::string> vars;
	size_t i = 0;
	while (i < src.size()) {
		size_t dashPos = src.find("--", i);
		if (dashPos == std::string_view::npos) break;

		size_t colonPos = src.find(':', dashPos);
		size_t semiPos  = src.find(';', dashPos);
		size_t closePos = src.find('}', dashPos);

		if (colonPos == std::string_view::npos || semiPos == std::string_view::npos) break;
		// Must be inside a block and before the next closing brace.
		if (colonPos > semiPos || colonPos > closePos) { i = dashPos + 2; continue; }

		std::string name  = ParserHelper::Trim(src.substr(dashPos, colonPos - dashPos));
		std::string value = ParserHelper::Trim(src.substr(colonPos + 1, semiPos - colonPos - 1));

		if (name.size() > 2 && name[0] == '-' && name[1] == '-') {
			vars[std::move(name)] = std::move(value);
		}
		i = semiPos + 1;
	}
	return vars;
}

// Replace every var(--name) occurrence with the resolved value.
static std::string SubstituteVariables(std::string src,
                                       const std::unordered_map<std::string, std::string> &vars) {
	for (const auto &[name, value] : vars) {
		std::string needle = "var(" + name + ")";
		size_t pos = 0;
		while ((pos = src.find(needle, pos)) != std::string::npos) {
			src.replace(pos, needle.size(), value);
			pos += value.size();
		}
	}
	return src;
}

// Parse a single condition like "width < 900px" or "min-width: 600px".
static Option<MediaCondition> ParseMediaCondition(std::string_view raw) {
	using Axis = MediaCondition::Axis;
	using Op   = MediaCondition::Op;

	const std::string c = ParserHelper::Trim(raw);
	if (c.empty()) return std::nullopt;

	// CSS standard: min-width/max-width/min-height/max-height: VALUE
	const size_t colonPos = c.find(':');
	if (colonPos != std::string::npos) {
		const std::string prop = ParserHelper::Trim(c.substr(0, colonPos));
		const float val = ParserHelper::ParseFloat(ParserHelper::Trim(c.substr(colonPos + 1)));
		if (prop == "min-width")  return MediaCondition{ Axis::Width,  Op::GreaterEq, val };
		if (prop == "max-width")  return MediaCondition{ Axis::Width,  Op::LessEq,    val };
		if (prop == "min-height") return MediaCondition{ Axis::Height, Op::GreaterEq, val };
		if (prop == "max-height") return MediaCondition{ Axis::Height, Op::LessEq,    val };
		if (prop == "width")      return MediaCondition{ Axis::Width,  Op::Equal,     val };
		if (prop == "height")     return MediaCondition{ Axis::Height, Op::Equal,     val };
		return std::nullopt;
	}

	// Range syntax: "width < 900px", "height >= 600px", etc.
	Axis axis;
	size_t axisEnd = 0;
	if (c.size() >= 6 && c.substr(0, 6) == "height") { axis = Axis::Height; axisEnd = 6; }
	else if (c.size() >= 5 && c.substr(0, 5) == "width") { axis = Axis::Width; axisEnd = 5; }
	else return std::nullopt;

	const std::string rest = ParserHelper::Trim(c.substr(axisEnd));
	if (rest.empty()) return std::nullopt;

	Op op;
	size_t opEnd = 0;
	if (rest.size() >= 2 && rest.substr(0, 2) == "<=") { op = Op::LessEq;    opEnd = 2; }
	else if (rest.size() >= 2 && rest.substr(0, 2) == ">=") { op = Op::GreaterEq; opEnd = 2; }
	else if (rest[0] == '<') { op = Op::Less;    opEnd = 1; }
	else if (rest[0] == '>') { op = Op::Greater; opEnd = 1; }
	else return std::nullopt;

	const float val = ParserHelper::ParseFloat(ParserHelper::Trim(rest.substr(opEnd)));
	return MediaCondition{ axis, op, val };
}

// Parse the condition text after "@media" or "@container" and all inner rules,
// then register a MediaBlock on the sheet.
static void ParseAtBlock(std::string_view condText, std::string_view body,
                         StyleSheet &sheet, bool isContainer) {
	// Parse conditions separated by "and".
	std::vector<MediaCondition> conditions;
	const std::string condStr = ParserHelper::Trim(condText);
	size_t j = 0;
	while (j < condStr.size()) {
		while (j < condStr.size() && std::isspace((unsigned char)condStr[j])) ++j;
		if (j >= condStr.size()) break;

		if (condStr[j] == '(') {
			const size_t closeP = condStr.find(')', j);
			if (closeP == std::string::npos) break;
			auto cond = ParseMediaCondition(std::string_view(condStr).substr(j + 1, closeP - j - 1));
			if (cond) conditions.push_back(*cond);
			j = closeP + 1;
		} else {
			// Skip "and" keyword and anything else between parens.
			while (j < condStr.size() && condStr[j] != '(') ++j;
		}
	}
	if (conditions.empty()) return;

	// Parse the inner rules (standard single-level CSS blocks).
	StyleSheet tempSheet;
	size_t k = 0;
	while (k < body.size()) {
		while (k < body.size() && std::isspace((unsigned char)body[k])) ++k;
		if (k >= body.size()) break;

		const size_t innerOpen = body.find('{', k);
		if (innerOpen == std::string_view::npos) break;

		const std::string innerSel = ParserHelper::Trim(body.substr(k, innerOpen - k));

		const size_t innerClose = body.find('}', innerOpen + 1);
		if (innerClose == std::string_view::npos) break;

		if (!innerSel.empty() && innerSel[0] != '@') {
			const std::string_view innerBody = body.substr(innerOpen + 1, innerClose - innerOpen - 1);
			ParserHelper::ParseBlock(innerSel, innerBody, tempSheet);
		}
		k = innerClose + 1;
	}

	MediaBlock block;
	block.conditions = std::move(conditions);
	block.rules      = tempSheet.GetRules();

	if (isContainer) sheet.AddContainerBlock(std::move(block));
	else             sheet.AddMediaBlock(std::move(block));
}

// ── Public API ────────────────────────────────────────────────────────────────

void StyleParser::ApplyProperty(StyleProperties &props, std::string_view property, std::string_view value) {
	ParserHelper::ApplyDeclaration(props, property, value);
}

bool StyleParser::LoadFile(const std::string &path, StyleSheet &sheet) {
	const std::string src = Platform::Filesystem::VirtualFileSystem::Get()->ReadTextFile(path);
	if (src.empty()) {
		AQUILA_LOG_ERROR("StyleParser: cannot open '{}'", path);
		return false;
	}
	return LoadString(src, sheet);
}

bool StyleParser::LoadString(std::string_view css, StyleSheet &sheet) {
	std::string src = ParserHelper::StripComments(css);

	// Pass 1 — collect CSS custom properties (--name: value) and store them.
	auto vars = ExtractVariables(src);
	for (const auto &[name, value] : vars) {
		sheet.AddVariable(name, value);
	}

	// Pass 2 — substitute var(--name) references throughout the source text.
	if (!vars.empty()) {
		src = SubstituteVariables(std::move(src), vars);
	}

	// Pass 3 — parse all blocks.
	size_t i = 0;
	while (i < src.size()) {
		while (i < src.size() && std::isspace(static_cast<unsigned char>(src[i]))) ++i;
		if (i >= src.size()) break;

		const size_t braceOpen = src.find('{', i);
		if (braceOpen == std::string::npos) break;

		const std::string selector = ParserHelper::Trim(std::string_view(src).substr(i, braceOpen - i));

		// Use depth-aware matching so @media bodies with nested {} are consumed correctly.
		const size_t braceClose = FindMatchingClose(src, braceOpen);
		if (braceClose == std::string::npos) {
			AQUILA_LOG_ERROR("StyleParser: unclosed block for selector '{}'", selector);
			return false;
		}

		const std::string_view body = std::string_view(src).substr(braceOpen + 1, braceClose - braceOpen - 1);

		if (!selector.empty()) {
			if (selector.size() > 6 && selector.substr(0, 6) == "@media") {
				ParseAtBlock(std::string_view(selector).substr(6), body, sheet, false);
			} else if (selector.size() > 10 && selector.substr(0, 10) == "@container") {
				ParseAtBlock(std::string_view(selector).substr(10), body, sheet, true);
			} else if (selector[0] != '@') {
				ParserHelper::ParseBlock(selector, body, sheet);
			}
			// Unknown at-rules are silently skipped.
		}

		i = braceClose + 1;
	}

	return true;
}

} // namespace Aquila::UI
