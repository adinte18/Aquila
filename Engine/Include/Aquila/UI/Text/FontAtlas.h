#pragma once

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"

#include <string>
#include <unordered_map>

namespace Aquila::UI::Text {

struct GlyphInfo {
	vec2 uvMin;
	vec2 uvMax;
	vec2 size;
	vec2 bearing;
	float advance;
};

class FontAtlas {
  public:
	~FontAtlas();

	AQUILA_NONCOPYABLE(FontAtlas);
	AQUILA_NONMOVEABLE(FontAtlas);

	static Unique<FontAtlas> Create(GFX::GfxContext &ctx, const uint8 *ttfData, uint64 dataSize, float pixelHeight);
	static Unique<FontAtlas> CreateFromFile(GFX::GfxContext &ctx, const std::string &path, float pixelHeight);

	const GlyphInfo *GetGlyph(uint32 codepoint) const;
	GFX::GfxTexture *GetTexture() const { return m_Texture.get(); }

	float GetLineHeight() const { return m_LineHeight; }
	float GetAscent() const { return m_Ascent; }
	float GetDescent() const { return m_Descent; }

  private:
	FontAtlas() = default;
	GFX::GfxContext *m_Ctx = nullptr;
	Ref<GFX::GfxTexture> m_Texture;
	std::unordered_map<uint32, GlyphInfo> m_Glyphs;
	float m_LineHeight = 0.f;
	float m_Ascent = 0.f;
	float m_Descent = 0.f;
};

} // namespace Aquila::UI::Text
