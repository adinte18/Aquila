#include "Aquila/UI/Style/Theme.h"
#include "Aquila/UI/Style/StyleSheet.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include <cstdio>

namespace Aquila::UI {

void Theme::Set(std::string_view typeName, StyleProperties props) {
	m_Styles[{ std::string(typeName), "" }] = std::move(props);
}

void Theme::Set(std::string_view typeName, std::string_view pseudoClass, StyleProperties props) {
	m_Styles[{ std::string(typeName), std::string(pseudoClass) }] = std::move(props);
}

void Theme::SetColor(std::string_view name, vec4 color) {
	m_Colors[std::string(name)] = color;
}

void Theme::SetConstant(std::string_view name, float value) {
	m_Constants[std::string(name)] = value;
}

Option<vec4> Theme::GetColor(std::string_view name) const {
	const auto it = m_Colors.find(std::string(name));
	return it != m_Colors.end() ? Option<vec4>(it->second) : std::nullopt;
}

Option<float> Theme::GetConstant(std::string_view name) const {
	const auto it = m_Constants.find(std::string(name));
	return it != m_Constants.end() ? Option<float>(it->second) : std::nullopt;
}

const StyleProperties *Theme::Get(std::string_view typeName, std::string_view pseudoClass) const {
	const auto it = m_Styles.find({ std::string(typeName), std::string(pseudoClass) });
	return it != m_Styles.end() ? &it->second : nullptr;
}

void Theme::ApplyToStyleSheet(StyleSheet &sheet) const {
	for (const auto &[key, props] : m_Styles) {
		const StyleRule::SelectorType type = StyleRule::SelectorType::Type;
		sheet.AddRule(type, key.typeName, key.pseudoClass, props);
	}
}

static std::string FmtFloat(float v) {
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%.4g", v);
	return buf;
}

static std::string FmtColor(const vec4 &c) {
	char buf[64];
	std::snprintf(buf, sizeof(buf), "rgba(%.3f, %.3f, %.3f, %.3f)", std::clamp(c.r, 0.f, 1.f),
				  std::clamp(c.g, 0.f, 1.f), std::clamp(c.b, 0.f, 1.f), std::clamp(c.a, 0.f, 1.f));
	return buf;
}

static std::string FmtLength(const StyleLength &l) {
	char buf[32];
	switch (l.unit) {
	case LengthUnit::Auto:
		return "auto";
	case LengthUnit::Grow:
		return "grow";
	case LengthUnit::Percent:
		std::snprintf(buf, sizeof(buf), "%.4g%%", l.value);
		return buf;
	default:
		std::snprintf(buf, sizeof(buf), "%.4gpx", l.value);
		return buf;
	}
}

static std::string FmtEdges(const StyleEdges &e) {
	if (e.top == e.right && e.right == e.bottom && e.bottom == e.left) {
		return FmtLength(e.top);
	}
	if (e.top == e.bottom && e.left == e.right) {
		return FmtLength(e.top) + " " + FmtLength(e.right);
	}
	return FmtLength(e.top) + " " + FmtLength(e.right) + " " + FmtLength(e.bottom) + " " + FmtLength(e.left);
}

static void Decl(std::string &out, std::string_view prop, std::string_view value) {
	out += "    ";
	out += prop;
	out += ": ";
	out += value;
	out += ";\n";
}

static void AppendDeclarations(std::string &out, const StyleProperties &p) {
	if (p.backgroundColor) {
		Decl(out, "background-color", FmtColor(*p.backgroundColor));
	}
	if (p.borderColor) {
		Decl(out, "border-color", FmtColor(*p.borderColor));
	}
	if (p.color) {
		Decl(out, "color", FmtColor(*p.color));
	}
	if (p.borderWidth) {
		Decl(out, "border-width", FmtFloat(*p.borderWidth) + "px");
	}
	if (p.borderRadius) {
		const vec4 &r = *p.borderRadius;
		std::string v;
		if (r.x == r.y && r.y == r.z && r.z == r.w) {
			v = FmtFloat(r.x) + "px";
		} else {
			v = FmtFloat(r.x) + "px " + FmtFloat(r.y) + "px " + FmtFloat(r.z) + "px " + FmtFloat(r.w) + "px";
		}
		Decl(out, "border-radius", v);
	}
	if (p.opacity) {
		Decl(out, "opacity", FmtFloat(*p.opacity));
	}

	if (p.display) {
		Decl(out, "display", *p.display == Display::Flex ? "flex" : "none");
	}
	if (p.overflow) {
		const char *v = "visible";
		if (*p.overflow == Overflow::Hidden) {
			v = "hidden";
		} else if (*p.overflow == Overflow::Scroll) {
			v = "scroll";
		}
		Decl(out, "overflow", v);
	}

	if (p.width) {
		Decl(out, "width", FmtLength(*p.width));
	}
	if (p.height) {
		Decl(out, "height", FmtLength(*p.height));
	}
	if (p.min) {
		Decl(out, "min", FmtLength(*p.min));
	}
	if (p.max) {
		Decl(out, "max", FmtLength(*p.max));
	}
	if (p.minWidth) {
		Decl(out, "min-width", FmtLength(*p.minWidth));
	}
	if (p.maxWidth) {
		Decl(out, "max-width", FmtLength(*p.maxWidth));
	}
	if (p.minHeight) {
		Decl(out, "min-height", FmtLength(*p.minHeight));
	}
	if (p.maxHeight) {
		Decl(out, "max-height", FmtLength(*p.maxHeight));
	}

	if (p.padding) {
		Decl(out, "padding", FmtEdges(*p.padding));
	}
	if (p.gap) {
		Decl(out, "gap", FmtFloat(*p.gap) + "px");
	}

	if (p.flexDirection) {
		const char *v = "row";
		switch (*p.flexDirection) {
		case FlexDirection::Column:
			v = "column";
			break;
		case FlexDirection::RowReverse:
			v = "row-reverse";
			break;
		case FlexDirection::ColumnReverse:
			v = "column-reverse";
			break;
		default:
			break;
		}
		Decl(out, "flex-direction", v);
	}
	if (p.justifyContent) {
		const char *v = "start";
		if (*p.justifyContent == JustifyContent::Center) {
			v = "center";
		} else if (*p.justifyContent == JustifyContent::End) {
			v = "end";
		}
		Decl(out, "justify-content", v);
	}
	if (p.alignItems) {
		const char *v = "start";
		switch (*p.alignItems) {
		case AlignItems::End:
			v = "end";
			break;
		case AlignItems::Center:
			v = "center";
			break;
		case AlignItems::Stretch:
			v = "stretch";
			break;
		default:
			break;
		}
		Decl(out, "align-items", v);
	}
	if (p.flexGrow) {
		Decl(out, "flex-grow", FmtFloat(*p.flexGrow));
	}
	if (p.flexWrap) {
		Decl(out, "flex-wrap", *p.flexWrap == FlexWrap::Wrap ? "wrap" : "nowrap");
	}

	if (p.position) {
		const char *v = "static";
		if (*p.position == Position::Relative) {
			v = "relative";
		} else if (*p.position == Position::Absolute) {
			v = "absolute";
		}
		Decl(out, "position", v);
	}
	if (p.top) {
		Decl(out, "top", FmtLength(*p.top));
	}
	if (p.right) {
		Decl(out, "right", FmtLength(*p.right));
	}
	if (p.bottom) {
		Decl(out, "bottom", FmtLength(*p.bottom));
	}
	if (p.left) {
		Decl(out, "left", FmtLength(*p.left));
	}
	if (p.zIndex) {
		Decl(out, "z-index", std::to_string(*p.zIndex));
	}

	if (p.fontFamily) {
		Decl(out, "font-family", *p.fontFamily);
	}
	if (p.fontSize) {
		Decl(out, "font-size", FmtFloat(*p.fontSize));
	}
	if (p.textAlign) {
		const char *v = "left";
		if (*p.textAlign == TextAlign::Center) {
			v = "center";
		} else if (*p.textAlign == TextAlign::Right) {
			v = "right";
		}
		Decl(out, "text-align", v);
	}

	if (p.transitionDuration) {
		char buf[32];
		std::snprintf(buf, sizeof(buf), "%.4gms", *p.transitionDuration);
		Decl(out, "transition-duration", buf);
	}
	if (p.transitionEasing) {
		const char *v = "linear";
		switch (*p.transitionEasing) {
		case TransitionEasing::Ease:
			v = "ease";
			break;
		case TransitionEasing::EaseIn:
			v = "ease-in";
			break;
		case TransitionEasing::EaseOut:
			v = "ease-out";
			break;
		case TransitionEasing::EaseInOut:
			v = "ease-in-out";
			break;
		default:
			break;
		}
		Decl(out, "transition-easing", v);
	}

	if (p.boxShadows && !p.boxShadows->empty()) {
		std::string value;
		for (size_t i = 0; i < p.boxShadows->size(); ++i) {
			if (i > 0) {
				value += ", ";
			}
			const BoxShadow &sh = (*p.boxShadows)[i];
			if (sh.inset) {
				value += "inset ";
			}
			value += FmtFloat(sh.offset.x) + "px ";
			value += FmtFloat(sh.offset.y) + "px ";
			value += FmtFloat(sh.blur) + "px ";
			value += FmtFloat(sh.spread) + "px ";
			value += FmtColor(sh.color);
		}
		Decl(out, "box-shadow", value);
	}
}

std::string Theme::ToAqStyle() const {
	std::string out;
	out += "/* Generated from Theme — load via StyleParser::LoadString() or LoadFile() */\n";

	if (!m_Colors.empty()) {
		out += "\n/* @palette — reference only, not loaded by the parser */\n";
		std::vector<std::pair<std::string, vec4>> colors(m_Colors.begin(), m_Colors.end());
		std::ranges::sort(colors, {}, &std::pair<std::string, vec4>::first);
		for (const auto &[name, color] : colors) {
			char buf[128];
			std::snprintf(buf, sizeof(buf), "/* @color %-24s %s */\n", (name + ":").c_str(), FmtColor(color).c_str());
			out += buf;
		}
	}

	if (!m_Constants.empty()) {
		out += "\n/* @constants — reference only, not loaded by the parser */\n";
		std::vector<std::pair<std::string, float>> constants(m_Constants.begin(), m_Constants.end());
		std::ranges::sort(constants, {}, &std::pair<std::string, float>::first);
		for (const auto &[name, value] : constants) {
			char buf[64];
			std::snprintf(buf, sizeof(buf), "/* @const %-24s %.4g */\n", (name + ":").c_str(), value);
			out += buf;
		}
	}

	using Entry = const decltype(m_Styles)::value_type *;
	std::vector<Entry> entries;
	entries.reserve(m_Styles.size());
	for (const auto &kv : m_Styles) {
		entries.push_back(&kv);
	}
	std::ranges::sort(entries, [](Entry a, Entry b) {
		if (a->first.typeName != b->first.typeName) {
			return a->first.typeName < b->first.typeName;
		}
		return a->first.pseudoClass < b->first.pseudoClass;
	});

	for (const auto *entry : entries) {
		const auto &[key, props] = *entry;
		out += "\n";
		out += key.typeName;
		if (!key.pseudoClass.empty()) {
			out += ":";
			out += key.pseudoClass;
		}
		out += " {\n";
		AppendDeclarations(out, props);
		out += "}\n";
	}

	return out;
}

bool Theme::SaveToFile(const std::string &path) const {
	return Platform::Filesystem::VirtualFileSystem::Get()->WriteTextFile(path, ToAqStyle());
}

} // namespace Aquila::UI
