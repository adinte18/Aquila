#pragma once
#include "stb/stb_truetype.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Math/Geometry/Bezier.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/UI/Text/FontAtlasBuilder.h"

namespace Aquila::UI::Text {

// CPU-side per-glyph placement data (used by DrawList for cursor math).
struct GlyphInfo {
	uint32 glyphID; // index into FontAtlas::m_SlugGlyphs
	vec2 size;		// glyph size in pixels at bake scale
	vec2 bearing;	// offset from cursor baseline to top-left of quad (screen Y-down)
	f32 advance;
};

// Per-glyph Slug vertex data — constant across all 4 vertices of the glyph quad.
struct SlugGlyphData {
	// Band texture location for this glyph's header row.
	uint32 glyphLocX;
	uint32 glyphLocY;

	// Maximum band indices (0-based; 16 bands → bandMax = 15).
	uint32 bandMaxX;
	uint32 bandMaxY;

	// Band transform: bandIndex = renderCoord * scale + offset.
	vec4 bandTransform; // (scaleX, scaleY, offsetX, offsetY)

	// Em-space extents in Y-UP glyph-local coordinates (origin at bottom-left of bbox).
	vec2 emMin; // always (0, 0) for current glyphs
	vec2 emMax; // (glyphW, glyphH) at bake scale
};


class FontAtlas {
  public:
	~FontAtlas() = default;
	AQUILA_NONCOPYABLE(FontAtlas);
	AQUILA_NONMOVEABLE(FontAtlas);

	static Unique<FontAtlas> Create(GFX::GfxContext &ctx, const uint8 *ttfData, uint64 dataSize, f32 pixelHeight);

	static Unique<FontAtlas> CreateFromFile(GFX::GfxContext &ctx, const std::string &path, f32 pixelHeight);

	const GlyphInfo *GetGlyph(uint32 codepoint) const;
	const SlugGlyphData *GetSlugData(uint32 glyphID) const;

	GFX::GfxTexture *GetCurveTexture() const { return m_CurveTexture.get(); }
	GFX::GfxTexture *GetBandTexture() const { return m_BandTexture.get(); }

	f32 GetLineHeight() const { return m_LineHeight; }
	f32 GetAscent() const { return m_Ascent; }
	f32 GetDescent() const { return m_Descent; }
	f32 GetBakeSize() const { return m_BakeSize; }

  private:
	FontAtlas() = default;

	static void BuildGlyphCurves(const stbtt_fontinfo &fontInfo, int glyphIndex, f32 scale, GlyphBuild &out);
	static GlyphInfo BuildGlyphInfo(const stbtt_fontinfo &fontInfo, int glyphIndex, uint32 glyphID, f32 scale);

	GFX::GfxContext *m_Ctx = nullptr;

	Ref<GFX::GfxTexture> m_CurveTexture; // RGBA32F: 2 texels per curve (p0+p1, p2)
	Ref<GFX::GfxTexture> m_BandTexture;	 // RGBA32U: band headers + curve index lists

	std::unordered_map<uint32, GlyphInfo> m_Glyphs;
	std::vector<SlugGlyphData> m_SlugGlyphs;

	f32 m_LineHeight = 0.f;
	f32 m_Ascent = 0.f;
	f32 m_Descent = 0.f;
	f32 m_BakeSize = 0.f;
};

} // namespace Aquila::UI::Text
