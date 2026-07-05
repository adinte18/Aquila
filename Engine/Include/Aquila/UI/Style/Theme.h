#pragma once

#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Style/StyleSheet.h"

namespace Aquila::UI {

class Theme {
  public:
	Theme() = default;

	// Base style for a widget type (applied before any CSS rules).
	void set(std::string_view type_name, StyleProperties props);
	// State variant — pseudoClass: "hover" | "pressed" | "focus".
	void set(std::string_view type_name, std::string_view pseudo_class, StyleProperties props);

	// Named colour tokens — useful for consistent palette access from C++.
	void set_color(std::string_view name, Vec4 color);
	void set_constant(std::string_view name, float value);

	[[nodiscard]] Option<Vec4> get_color(std::string_view name) const;
	[[nodiscard]] Option<float> get_constant(std::string_view name) const;

	// Returns nullptr if no entry exists for (typeName, pseudoClass).
	[[nodiscard]] const StyleProperties *get(std::string_view type_name, std::string_view pseudo_class = "") const;

	// Directly injects every theme entry as StyleRule objects into |sheet|.
	// This is the preferred runtime path — no text serialisation or re-parsing.
	void apply_to_style_sheet(StyleSheet &sheet) const;

	// Serialise the theme to .aqstyle text. Useful for save/export; not needed for runtime.
	// Color/constant tokens are emitted as comments (reference only).
	[[nodiscard]] std::string to_aq_style() const;

	// Convenience wrapper: writes ToAqStyle() to a file. Returns false on error.
	bool save_to_file(const std::string &path) const;

  private:
	struct EntryKey {
		std::string type_name;
		std::string pseudo_class;
		bool operator==(const EntryKey &) const = default;
	};
	struct EntryKeyHash {
		size_t operator()(const EntryKey &k) const noexcept {
			return std::hash<std::string>{}(k.type_name) ^ (std::hash<std::string>{}(k.pseudo_class) << 16);
		}
	};

	std::unordered_map<EntryKey, StyleProperties, EntryKeyHash> m_styles;
	std::unordered_map<std::string, Vec4> m_colors;
	std::unordered_map<std::string, float> m_constants;
};

} // namespace Aquila::UI
