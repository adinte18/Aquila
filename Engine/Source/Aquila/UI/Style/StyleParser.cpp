
#include "Aquila/UI/Style/StyleParser.h"
#include "Aquila/UI/Style/StyleSheet.h"

namespace Aquila::UI {

void StyleParser::ApplyProperty(StyleProperties &props, std::string_view property, std::string_view value) {
	ParserHelper::ApplyDeclaration(props, property, value);
}

bool StyleParser::LoadFile(const std::string &path, StyleSheet &sheet) {
	std::ifstream f(path);
	if (!f.is_open()) {
		AQUILA_LOG_ERROR("StyleParser: cannot open '{}'", path);
		return false;
	}
	std::ostringstream ss;
	ss << f.rdbuf();
	return LoadString(ss.str(), sheet);
}

bool StyleParser::LoadString(std::string_view css, StyleSheet &sheet) {
	std::string src = ParserHelper::StripComments(css);
	size_t i = 0;

	while (i < src.size()) {
		while (i < src.size() && std::isspace(static_cast<unsigned char>(src[i]))) {
			++i;
		}
		if (i >= src.size()) {
			break;
		}

		size_t braceOpen = src.find('{', i);
		if (braceOpen == std::string::npos) {
			break;
		}

		std::string selector = ParserHelper::Trim(std::string_view(src).substr(i, braceOpen - i));

		size_t braceClose = src.find('}', braceOpen + 1);
		if (braceClose == std::string::npos) {
			AQUILA_LOG_ERROR("StyleParser: unclosed block for selector '{}'", selector);
			return false;
		}

		if (!selector.empty()) {
			std::string_view body = std::string_view(src).substr(braceOpen + 1, braceClose - braceOpen - 1);
			ParserHelper::ParseBlock(selector, body, sheet);
		}

		i = braceClose + 1;
	}

	return true;
}

} // namespace Aquila::UI
