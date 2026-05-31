#pragma once

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Style/StyleLength.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Style/StyleSheet.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Aquila::UI::ParserHelper {

std::string_view TrimSV(std::string_view s);
std::string Trim(std::string_view s);
std::string ToLower(std::string_view s);

std::vector<std::string> Split(std::string_view s, char delim);

std::vector<std::string> SplitWS(std::string_view s);

std::string StripComments(std::string_view src);

float ParseFloat(std::string_view s);

float ParseDurationMs(std::string_view s);

std::optional<TransitionEasing> ParseEasing(std::string_view s);
Option<StyleLength> ParseLength(std::string_view raw);
Option<vec4> ParseColor(std::string_view raw);

Option<StyleEdges> ParseEdges(std::string_view s);
Option<vec4> ParseRadius(std::string_view s);
Option<BoxShadow> ParseOneShadow(std::string_view raw);

void ApplyDeclaration(StyleProperties &props, std::string_view propRaw, std::string_view valueRaw);
void ParseBlock(std::string_view selectorList, std::string_view body, StyleSheet &sheet);

} // namespace Aquila::UI::ParserHelper
