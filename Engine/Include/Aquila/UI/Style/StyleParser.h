#pragma once

#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Style/StyleParserHelper.h"

namespace Aquila::UI {

class StyleSheet;

class StyleParser {
  public:
	static bool LoadFile(const std::string &path, StyleSheet &sheet);

	static bool LoadString(std::string_view css, StyleSheet &sheet);

	static void ApplyProperty(StyleProperties &props, std::string_view property, std::string_view value);
};

} // namespace Aquila::UI
