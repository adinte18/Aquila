#pragma once

#include "Aquila/UI/Text/FontAtlas.h"

namespace Aquila::UI::Core {

class FontRegistry {
  public:
	static void Register(const std::string &name, Text::FontAtlas *atlas) { GetMap()[name] = atlas; }

	static Text::FontAtlas *Resolve(const std::string &name) {
		const auto &map = GetMap();
		const auto it = map.find(name);
		return it != map.end() ? it->second : nullptr;
	}

  private:
	static std::unordered_map<std::string, Text::FontAtlas *> &GetMap() {
		static std::unordered_map<std::string, Text::FontAtlas *> map;
		return map;
	}
};

} // namespace Aquila::UI::Core
