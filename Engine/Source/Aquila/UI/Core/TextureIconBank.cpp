#include "Aquila/UI/Core/TextureIconBank.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::UI::Core {

void TextureIconBank::add_icon(const std::string &name, Vec2 uv_min, Vec2 uv_max) {
	m_icons[name] = IconEntry{ m_texture, uv_min, uv_max };
}

void TextureIconBank::add_icon_pixels(const std::string &name, float x, float y, float w, float h) {
	AQUILA_ASSERT(m_texture != nullptr, "TextureIconBank::AddIconPixels called before SetTexture");

	const float atlas_w = static_cast<float>(m_texture->get_width());
	const float atlas_h = static_cast<float>(m_texture->get_height());

	const Vec2 uv_min = { x / atlas_w, y / atlas_h };
	const Vec2 uv_max = { (x + w) / atlas_w, (y + h) / atlas_h };
	add_icon(name, uv_min, uv_max);
}

const IconEntry *TextureIconBank::get_icon(std::string_view name) const {
	auto it = m_icons.find(std::string(name));
	return (it != m_icons.end()) ? &it->second : nullptr;
}

} // namespace Aquila::UI::Core
