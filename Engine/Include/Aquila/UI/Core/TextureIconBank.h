#pragma once

#include "Aquila/Foundation/Defines.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

struct IconEntry {
	GFX::GfxTexture *texture = nullptr; // weak ref
	vec2 uvMin = { 0.f, 0.f };
	vec2 uvMax = { 1.f, 1.f };
};

class TextureIconBank {
  public:
	TextureIconBank() = default;

	void SetTexture(GFX::GfxTexture *texture) { m_Texture = texture; }
	[[nodiscard]] GFX::GfxTexture *GetTexture() const { return m_Texture; }

	void AddIcon(const std::string &name, vec2 uvMin, vec2 uvMax);

	void AddIconPixels(const std::string &name, float x, float y, float w, float h);

	[[nodiscard]] const IconEntry *GetIcon(std::string_view name) const;

	[[nodiscard]] bool IsEmpty() const { return m_Icons.empty(); }

  private:
	GFX::GfxTexture *m_Texture = nullptr;
	std::unordered_map<std::string, IconEntry> m_Icons;
};

} // namespace Aquila::UI::Core
