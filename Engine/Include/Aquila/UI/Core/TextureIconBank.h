#pragma once

#include "Aquila/Foundation/Defines.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

struct IconEntry {
	GFX::GfxTexture *texture = nullptr; // weak ref
	Vec2 uv_min = { 0.F, 0.F };
	Vec2 uv_max = { 1.F, 1.F };
};

class TextureIconBank {
  public:
	TextureIconBank() = default;

	void set_texture(GFX::GfxTexture *texture) { m_texture = texture; }
	[[nodiscard]] GFX::GfxTexture *get_texture() const { return m_texture; }

	void add_icon(const std::string &name, Vec2 uv_min, Vec2 uv_max);

	void add_icon_pixels(const std::string &name, float x, float y, float w, float h);

	[[nodiscard]] const IconEntry *get_icon(std::string_view name) const;

	[[nodiscard]] bool is_empty() const { return m_icons.empty(); }

  private:
	GFX::GfxTexture *m_texture = nullptr;
	std::unordered_map<std::string, IconEntry> m_icons;
};

} // namespace Aquila::UI::Core
