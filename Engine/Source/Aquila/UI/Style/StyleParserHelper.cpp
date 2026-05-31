#include "Aquila/UI/Style/StyleParserHelper.h"
#include "Aquila/Foundation/Macros.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>

namespace Aquila::UI::ParserHelper {

std::string_view TrimSV(std::string_view s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
        s.remove_prefix(1);
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
        s.remove_suffix(1);
    }
    return s;
}

std::string Trim(std::string_view s) {
    return std::string(TrimSV(s));
}

std::string ToLower(std::string_view s) {
    std::string out(s);
    for (char &c : out) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return out;
}

std::vector<std::string> Split(std::string_view s, char delim) {
    std::vector<std::string> result;
    while (!s.empty()) {
        size_t pos = s.find(delim);
        std::string_view tok = (pos == std::string_view::npos) ? s : s.substr(0, pos);
        std::string t = Trim(tok);
        if (!t.empty()) {
            result.push_back(std::move(t));
        }
        if (pos == std::string_view::npos) {
            break;
        }
        s.remove_prefix(pos + 1);
    }
    return result;
}

std::vector<std::string> SplitWS(std::string_view s) {
    std::vector<std::string> result;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
            ++i;
        }
        size_t start = i;
        while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i]))) {
            ++i;
        }
        if (i > start) {
            result.emplace_back(s.substr(start, i - start));
        }
    }
    return result;
}

std::string StripComments(std::string_view src) {
    std::string out;
    out.reserve(src.size());
    size_t i = 0;
    while (i < src.size()) {
        if (i + 1 < src.size() && src[i] == '/' && src[i + 1] == '*') {
            i += 2;
            while (i + 1 < src.size() && !(src[i] == '*' && src[i + 1] == '/')) {
                if (src[i] == '\n') {
                    out += '\n';
                }
                ++i;
            }
            i += 2;
        } else if (i + 1 < src.size() && src[i] == '/' && src[i + 1] == '/') {
            while (i < src.size() && src[i] != '\n') {
                ++i;
            }
        } else {
            out += src[i++];
        }
    }
    return out;
}

float ParseFloat(std::string_view s) {
    s = TrimSV(s);
    float v = 0.f;
    std::from_chars(s.data(), s.data() + s.size(), v);
    return v;
}

float ParseDurationMs(std::string_view s) {
    s = TrimSV(s);
    if (s.ends_with("ms")) {
        return ParseFloat(s.substr(0, s.size() - 2));
    }
    if (s.ends_with('s')) {
        return ParseFloat(s.substr(0, s.size() - 1)) * 1000.f;
    }
    return ParseFloat(s);
}

std::optional<TransitionEasing> ParseEasing(std::string_view s) {
    if (s == "linear")      return TransitionEasing::Linear;
    if (s == "ease")        return TransitionEasing::Ease;
    if (s == "ease-in")     return TransitionEasing::EaseIn;
    if (s == "ease-out")    return TransitionEasing::EaseOut;
    if (s == "ease-in-out") return TransitionEasing::EaseInOut;
    return std::nullopt;
}

Option<StyleLength> ParseLength(std::string_view raw) {
    std::string s = ToLower(Trim(raw));
    if (s == "auto")            return StyleLength::Auto();
    if (s == "grow" || s == "1fr") return StyleLength::Grow();
    if (s.ends_with("px"))      return StyleLength::Pixel(ParseFloat(std::string_view(s).substr(0, s.size() - 2)));
    if (s.ends_with("%"))       return StyleLength::Percent(ParseFloat(std::string_view(s).substr(0, s.size() - 1)));
    if (!s.empty() && (std::isdigit(static_cast<unsigned char>(s[0])) || s[0] == '-' || s[0] == '.')) {
        return StyleLength::Pixel(ParseFloat(s));
    }
    return std::nullopt;
}

Option<vec4> ParseColor(std::string_view raw) {
    std::string s = ToLower(Trim(raw));

    if (s.starts_with('#')) {
        auto hex = std::string_view(s).substr(1);
        auto byte = [&](size_t off) -> float {
            uint8_t v = 0;
            std::from_chars(hex.data() + off, hex.data() + off + 2, v, 16);
            return v / 255.f;
        };
        if (hex.size() == 6) return vec4(byte(0), byte(2), byte(4), 1.f);
        if (hex.size() == 8) return vec4(byte(0), byte(2), byte(4), byte(6));
        return std::nullopt;
    }

    bool hasAlpha = s.starts_with("rgba(");
    if (hasAlpha || s.starts_with("rgb(")) {
        size_t open  = s.find('(');
        size_t close = s.rfind(')');
        if (open == std::string::npos || close == std::string::npos) return std::nullopt;
        auto parts = Split(std::string_view(s).substr(open + 1, close - open - 1), ',');
        if (parts.size() < 3) return std::nullopt;
        float r = ParseFloat(parts[0]);
        float g = ParseFloat(parts[1]);
        float b = ParseFloat(parts[2]);
        float a = (hasAlpha && parts.size() >= 4) ? ParseFloat(parts[3]) : 1.f;
        if (r > 1.f || g > 1.f || b > 1.f) {
            r /= 255.f; g /= 255.f; b /= 255.f;
        }
        return vec4(r, g, b, a);
    }

    return std::nullopt;
}

Option<StyleEdges> ParseEdges(std::string_view s) {
    auto parts = SplitWS(s);
    if (parts.empty()) return std::nullopt;

    auto l0 = ParseLength(parts[0]);
    if (!l0) return std::nullopt;
    if (parts.size() == 1) return StyleEdges::All(*l0);

    auto l1 = ParseLength(parts[1]);
    if (!l1) return std::nullopt;
    if (parts.size() == 2) return StyleEdges::Axes(*l0, *l1);

    if (parts.size() >= 4) {
        auto l2 = ParseLength(parts[2]);
        auto l3 = ParseLength(parts[3]);
        if (!l2 || !l3) return std::nullopt;
        return StyleEdges{ .top = *l0, .right = *l1, .bottom = *l2, .left = *l3 };
    }

    return std::nullopt;
}

Option<vec4> ParseRadius(std::string_view s) {
    auto parts = SplitWS(s);
    if (parts.empty()) return std::nullopt;

    auto px = [](Option<StyleLength> l) -> float {
        return (l && l->unit == LengthUnit::Pixel) ? l->value : 0.f;
    };

    if (parts.size() == 1) {
        float v = px(ParseLength(parts[0]));
        return vec4(v);
    }
    if (parts.size() >= 4) {
        return vec4(px(ParseLength(parts[0])), px(ParseLength(parts[1])),
                    px(ParseLength(parts[2])), px(ParseLength(parts[3])));
    }
    return std::nullopt;
}

Option<BoxShadow> ParseOneShadow(std::string_view raw) {
    std::vector<std::string> tokens;
    size_t i = 0;
    std::string s = Trim(raw);
    while (i < s.size()) {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
            ++i;
        }
        if (i >= s.size()) break;
        size_t start = i;
        if (s[i] == 'r' && i + 3 < s.size() && s.substr(i, 3) == "rgb") {
            while (i < s.size() && s[i] != ')') { ++i; }
            if (i < s.size()) { ++i; }
        } else {
            while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i]))) { ++i; }
        }
        tokens.emplace_back(s.substr(start, i - start));
    }

    BoxShadow out;
    std::vector<std::string> lengths;
    for (const auto &tok : tokens) {
        if (tok == "inset") { out.inset = true; continue; }
        if (auto c = ParseColor(tok)) { out.color = *c; continue; }
        lengths.push_back(tok);
    }
    auto px = [](std::string_view v) -> float {
        std::string t = Trim(v);
        if (t.ends_with("px")) t = t.substr(0, t.size() - 2);
        float f = 0.f;
        std::from_chars(t.data(), t.data() + t.size(), f);
        return f;
    };
    if (lengths.size() < 2) return std::nullopt;
    out.offset.x = px(lengths[0]);
    out.offset.y = px(lengths[1]);
    if (lengths.size() >= 3) out.blur   = std::max(0.f, px(lengths[2]));
    if (lengths.size() >= 4) out.spread = px(lengths[3]);
    return out;
}

void ApplyDeclaration(StyleProperties &props, std::string_view propRaw, std::string_view valueRaw) {
    std::string prop       = ToLower(Trim(propRaw));
    std::string value      = Trim(valueRaw);
    std::string valueLower = ToLower(value);

    // CSS custom properties (--name) are handled by the variable extraction pass;
    // silently ignore them here so no spurious warnings are emitted.
    if (prop.size() >= 2 && prop[0] == '-' && prop[1] == '-') return;

    if (prop == "background-color" || prop == "background") {
        if (auto color = ParseColor(value)) props.backgroundColor = *color;
    } else if (prop == "border-color") {
        if (auto color = ParseColor(value)) props.borderColor = *color;
    } else if (prop == "color") {
        if (auto color = ParseColor(value)) props.color = *color;
    } else if (prop == "accent-color") {
        if (auto color = ParseColor(value)) props.accentColor = *color;

    // ── border ──
    } else if (prop == "border-width") {
        props.borderWidth = ParseFloat(value);
    } else if (prop == "border-radius") {
        if (auto radius = ParseRadius(value)) props.borderRadius = *radius;

    // ── opacity / display / overflow ──
    } else if (prop == "opacity") {
        props.opacity = ParseFloat(value);
    } else if (prop == "display") {
        if (valueLower == "flex")      props.display = Display::Flex;
        else if (valueLower == "none") props.display = Display::None;
    } else if (prop == "overflow") {
        if (valueLower == "visible")     props.overflow = Overflow::Visible;
        else if (valueLower == "hidden") props.overflow = Overflow::Hidden;
        else if (valueLower == "scroll") props.overflow = Overflow::Scroll;

    // ── sizing ──
    } else if (prop == "width") {
        if (auto l = ParseLength(value)) props.width = *l;
    } else if (prop == "height") {
        if (auto l = ParseLength(value)) props.height = *l;
    } else if (prop == "min-width") {
        if (auto l = ParseLength(value)) props.minWidth = *l;
    } else if (prop == "max-width") {
        if (auto l = ParseLength(value)) props.maxWidth = *l;
    } else if (prop == "min-height") {
        if (auto l = ParseLength(value)) props.minHeight = *l;
    } else if (prop == "max-height") {
        if (auto l = ParseLength(value)) props.maxHeight = *l;
    } else if (prop == "min") {
        if (auto l = ParseLength(value)) props.min = *l;
    } else if (prop == "max") {
        if (auto l = ParseLength(value)) props.max = *l;

    // ── box model ──
    } else if (prop == "gap") {
        if (auto l = ParseLength(value)) props.gap = l->unit == LengthUnit::Pixel ? l->value : 0.f;
    } else if (prop == "padding") {
        if (auto e = ParseEdges(value)) props.padding = *e;
    } else if (prop == "padding-left") {
        if (auto l = ParseLength(value)) props.paddingLeft = *l;
    } else if (prop == "padding-right") {
        if (auto l = ParseLength(value)) props.paddingRight = *l;
    } else if (prop == "padding-top") {
        if (auto l = ParseLength(value)) props.paddingTop = *l;
    } else if (prop == "padding-bottom") {
        if (auto l = ParseLength(value)) props.paddingBottom = *l;

    // ── flex ──
    } else if (prop == "flex-direction") {
        if (valueLower == "row")             props.flexDirection = FlexDirection::Row;
        else if (valueLower == "column")     props.flexDirection = FlexDirection::Column;
        else if (valueLower == "row-reverse")    props.flexDirection = FlexDirection::RowReverse;
        else if (valueLower == "column-reverse") props.flexDirection = FlexDirection::ColumnReverse;
    } else if (prop == "justify-content") {
        if (valueLower == "start" || valueLower == "flex-start")  props.justifyContent = JustifyContent::Start;
        else if (valueLower == "end"   || valueLower == "flex-end") props.justifyContent = JustifyContent::End;
        else if (valueLower == "center") props.justifyContent = JustifyContent::Center;
        else AQUILA_LOG_WARNING("JustifyContent : {} is unsupported", valueLower);
    } else if (prop == "align-items") {
        if (valueLower == "start" || valueLower == "flex-start")  props.alignItems = AlignItems::Start;
        else if (valueLower == "end"   || valueLower == "flex-end") props.alignItems = AlignItems::End;
        else if (valueLower == "center")  props.alignItems = AlignItems::Center;
        else if (valueLower == "stretch") props.alignItems = AlignItems::Stretch;
    } else if (prop == "flex-grow") {
        props.flexGrow = ParseFloat(value);
    } else if (prop == "flex-wrap") {
        if (valueLower == "nowrap" || valueLower == "no-wrap") props.flexWrap = FlexWrap::NoWrap;
        else if (valueLower == "wrap") props.flexWrap = FlexWrap::Wrap;

    } else if (prop == "position") {
        if (valueLower == "static")        props.position = Position::Static;
        else if (valueLower == "relative") props.position = Position::Relative;
        else if (valueLower == "absolute") props.position = Position::Absolute;
    } else if (prop == "top") {
        if (auto l = ParseLength(value)) props.top = *l;
    } else if (prop == "right") {
        if (auto l = ParseLength(value)) props.right = *l;
    } else if (prop == "bottom") {
        if (auto l = ParseLength(value)) props.bottom = *l;
    } else if (prop == "left") {
        if (auto l = ParseLength(value)) props.left = *l;
    } else if (prop == "z-index") {
        props.zIndex = static_cast<int32>(ParseFloat(value));

    } else if (prop == "font-family") {
        props.fontFamily = std::string(value);
    } else if (prop == "font-size") {
        static const std::unordered_map<std::string_view, FontSize> kFontSizeNames = {
            { "Tiny",        FontSize::Tiny        }, { "XSmall",      FontSize::XSmall      },
            { "Small",       FontSize::Small       }, { "Body",        FontSize::Body        },
            { "BodyLarge",   FontSize::BodyLarge   }, { "Subtitle",    FontSize::Subtitle    },
            { "Heading",     FontSize::Heading     }, { "HeadingLarge",FontSize::HeadingLarge },
            { "Title",       FontSize::Title       }, { "TitleLarge",  FontSize::TitleLarge  },
            { "Display",     FontSize::Display     }, { "DisplayLarge",FontSize::DisplayLarge },
        };
        if (auto it = kFontSizeNames.find(value); it != kFontSizeNames.end()) {
            props.fontSize = FontSizeToPixels(it->second);
        } else {
            props.fontSize = ParseFloat(value);
        }

    // ── transitions ──
    } else if (prop == "transition-duration") {
        props.transitionDuration = ParseDurationMs(value);
    } else if (prop == "transition-easing" || prop == "transition-timing-function") {
        if (auto easing = ParseEasing(value)) props.transitionEasing = easing;
    } else if (prop == "text-align") {
        if (valueLower == "left")        props.textAlign = TextAlign::Left;
        else if (valueLower == "center") props.textAlign = TextAlign::Center;
        else if (valueLower == "right")  props.textAlign = TextAlign::Right;

    } else if (prop == "box-shadow") {
        if (valueLower == "none") {
            props.boxShadows = std::vector<BoxShadow>{};
        } else {
            std::vector<BoxShadow> shadows;
            std::string layer;
            int depth = 0;
            for (char c : value) {
                if (c == '(')       { ++depth; layer += c; }
                else if (c == ')')  { --depth; layer += c; }
                else if (c == ',' && depth == 0) {
                    if (auto sh = ParseOneShadow(layer)) shadows.push_back(*sh);
                    layer.clear();
                } else { layer += c; }
            }
            if (!layer.empty()) {
                if (auto sh = ParseOneShadow(layer)) shadows.push_back(*sh);
            }
            if (!shadows.empty()) props.boxShadows = std::move(shadows);
        }

    } else if (prop == "transition") {
        for (std::string_view token : Split(value, ' ')) {
            token = TrimSV(token);
            if (token.empty() || token == "all") continue;
            if (auto easing = ParseEasing(token)) {
                props.transitionEasing = easing;
            } else {
                if (token.ends_with("ms") || token.ends_with('s') ||
                    std::isdigit(static_cast<unsigned char>(token.front()))) {
                    props.transitionDuration = ParseDurationMs(token);
                }
            }
        }

    } else {
        AQUILA_LOG_WARNING("StyleParser: unknown property '{}'", prop);
    }
}

void ParseBlock(std::string_view selectorList, std::string_view body, StyleSheet &sheet) {
    StyleProperties props;

    for (const auto &decl : Split(body, ';')) {
        size_t colon = decl.find(':');
        if (colon == std::string::npos) continue;
        ApplyDeclaration(props, std::string_view(decl).substr(0, colon),
                                std::string_view(decl).substr(colon + 1));
    }

    for (const auto &sel : Split(selectorList, ',')) {
        std::string s = Trim(sel);
        if (s.empty()) continue;

        std::string pseudoClass;
        size_t pseudoPos = s.find(':');
        if (pseudoPos != std::string::npos) {
            pseudoClass = ToLower(s.substr(pseudoPos + 1));
            s = s.substr(0, pseudoPos);
        }

        StyleRule::SelectorType type;
        std::string name;
        if (s.starts_with('#'))      { type = StyleRule::SelectorType::Id;    name = s.substr(1); }
        else if (s.starts_with('.')) { type = StyleRule::SelectorType::Class;  name = s.substr(1); }
        else                          { type = StyleRule::SelectorType::Type;   name = s; }

        sheet.AddRule(type, std::move(name), std::move(pseudoClass), props);
    }
}

} // namespace Aquila::UI::ParserHelper
