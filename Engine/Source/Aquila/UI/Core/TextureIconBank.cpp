#include "Aquila/UI/Core/TextureIconBank.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::UI::Core {

void TextureIconBank::AddIcon(const std::string &name, vec2 uvMin, vec2 uvMax) {
	m_Icons[name] = IconEntry{ m_Texture, uvMin, uvMax };
}

void TextureIconBank::AddIconPixels(const std::string &name, float x, float y, float w, float h) {
	AQUILA_ASSERT(m_Texture != nullptr, "TextureIconBank::AddIconPixels called before SetTexture");

	const float atlasW = static_cast<float>(m_Texture->GetWidth());
	const float atlasH = static_cast<float>(m_Texture->GetHeight());

	const vec2 uvMin = { x / atlasW, y / atlasH };
	const vec2 uvMax = { (x + w) / atlasW, (y + h) / atlasH };
	AddIcon(name, uvMin, uvMax);
}

const IconEntry *TextureIconBank::GetIcon(std::string_view name) const {
	auto it = m_Icons.find(std::string(name));
	return (it != m_Icons.end()) ? &it->second : nullptr;
}

} // namespace Aquila::UI::Core
