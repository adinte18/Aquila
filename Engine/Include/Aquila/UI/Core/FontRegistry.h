#pragma once

#include "Aquila/UI/Text/FontAtlas.h"

namespace Aquila::UI::Core {

class FontRegistry {
  public:
	static void Register(const std::string &name, Text::FontAtlas *atlas) { get_map()[name] = atlas; }

	static Text::FontAtlas *resolve(const std::string &name) {
		const auto &map = get_map();
		const auto it = map.find(name);
		return it != map.end() ? it->second : nullptr;
	}

	static void set_ui_scale(float scale) { get_scale() = scale; }
	static float ui_scale() { return get_scale(); }

  private:
	static std::unordered_map<std::string, Text::FontAtlas *> &get_map() {
		static std::unordered_map<std::string, Text::FontAtlas *> map;
		return map;
	}

	static float &get_scale() {
		static float scale = 1.0f;
		return scale;
	}
};

} // namespace Aquila::UI::Core
