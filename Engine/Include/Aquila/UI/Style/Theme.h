#pragma once

#include "Aquila/UI/Style/StyleProperties.h"

namespace Aquila::UI {

// Typed base-style bank per widget type, swappable at runtime.
//
// Resolution priority (lowest → highest):
//   Theme base style  <  Theme pseudo-class style  <  CSS rules  <  Inline style
//
// Usage:
//   auto theme = CreateRef<Theme>();
//   theme->Set("Button", StyleProperties{ .backgroundColor = ... });
//   theme->Set("Button", "hover", StyleProperties{ .backgroundColor = ... });
//   canvas.SetTheme(theme);
class Theme {
  public:
	Theme() = default;

	// Base style for a widget type (applied before any CSS rules).
	void Set(std::string_view typeName, StyleProperties props);
	// State variant — pseudoClass: "hover" | "pressed" | "focus".
	void Set(std::string_view typeName, std::string_view pseudoClass, StyleProperties props);

	// Named colour tokens — useful for consistent palette access from C++.
	void SetColor(std::string_view name, vec4 color);
	void SetConstant(std::string_view name, float value);

	[[nodiscard]] Option<vec4> GetColor(std::string_view name) const;
	[[nodiscard]] Option<float> GetConstant(std::string_view name) const;

	// Returns nullptr if no entry exists for (typeName, pseudoClass).
	[[nodiscard]] const StyleProperties *Get(std::string_view typeName, std::string_view pseudoClass = "") const;

	// Serialise the theme to .aqstyle text. The result can be passed directly to
	// StyleParser::LoadString() to produce an equivalent StyleSheet.
	// Color/constant tokens are emitted as comments (reference only).
	[[nodiscard]] std::string ToAqStyle() const;

	// Convenience wrapper: writes ToAqStyle() to a file. Returns false on error.
	bool SaveToFile(const std::string &path) const;

  private:
	struct EntryKey {
		std::string typeName;
		std::string pseudoClass;
		bool operator==(const EntryKey &) const = default;
	};
	struct EntryKeyHash {
		size_t operator()(const EntryKey &k) const noexcept {
			return std::hash<std::string>{}(k.typeName) ^ (std::hash<std::string>{}(k.pseudoClass) << 16);
		}
	};

	std::unordered_map<EntryKey, StyleProperties, EntryKeyHash> m_Styles;
	std::unordered_map<std::string, vec4> m_Colors;
	std::unordered_map<std::string, float> m_Constants;
};

} // namespace Aquila::UI
