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

#include <string_view>
#include <unordered_set>

namespace Aquila::UI::Text {

// CPU-side per-glyph placement data (used by DrawList for cursor math).
struct GlyphInfo {
	Uint32 glyph_id; // index into FontAtlas::m_SlugGlyphs
	Vec2 size; // glyph size in pixels at bake scale
	Vec2 bearing; // offset from cursor baseline to top-left of quad (screen Y-down)
	F32 advance;
};

// Per-glyph Slug vertex data — constant across all 4 vertices of the glyph quad.
struct SlugGlyphData {
	// Band texture location for this glyph's header row.
	Uint32 glyph_loc_x;
	Uint32 glyph_loc_y;

	// Maximum band indices (0-based; 16 bands → bandMax = 15).
	Uint32 band_max_x;
	Uint32 band_max_y;

	// Band transform: bandIndex = renderCoord * scale + offset.
	Vec4 band_transform; // (scaleX, scaleY, offsetX, offsetY)

	// Em-space extents in Y-UP glyph-local coordinates (origin at bottom-left of bbox).
	Vec2 em_min; // always (0, 0) for current glyphs
	Vec2 em_max; // (glyphW, glyphH) at bake scale
};

class FontAtlas {
  public:
	~FontAtlas() = default;
	AQUILA_NONCOPYABLE(FontAtlas);
	AQUILA_NONMOVEABLE(FontAtlas);

	static Unique<FontAtlas> create(GFX::GfxContext &ctx, const Uint8 *ttf_data, Uint64 data_size, F32 pixel_height);

	static Unique<FontAtlas> create_from_file(GFX::GfxContext &ctx, const std::string &path, F32 pixel_height);

	const GlyphInfo *get_glyph(Uint32 codepoint) const;
	const SlugGlyphData *get_slug_data(Uint32 glyph_id) const;

	// Loads any codepoints in `text` that are not yet resident, then re-uploads the
	// GPU textures if anything was added. Must be called on the main thread outside a
	// render pass (it submits an immediate copy). No-op when every glyph is present.
	void ensure_glyphs(std::string_view text);

	GFX::GfxTexture *get_curve_texture() const { return m_curve_texture.get(); }
	GFX::GfxTexture *get_band_texture() const { return m_band_texture.get(); }

	F32 get_line_height() const { return m_line_height; }
	F32 get_ascent() const { return m_ascent; }
	F32 get_descent() const { return m_descent; }
	F32 get_bake_size() const { return m_bake_size; }

  private:
	FontAtlas() = default;

	static void build_glyph_curves(const stbtt_fontinfo &font_info, int glyph_index, F32 scale, GlyphBuild &out);
	static GlyphInfo build_glyph_info(const stbtt_fontinfo &font_info, int glyph_index, Uint32 glyph_id, F32 scale);

	// Appends one glyph's curves + bands into the CPU shadow arrays. Returns true when a
	// glyph was actually added (false if already resident, absent from the font, or the
	// reserved capacity is exhausted). Does not touch the GPU — see ReuploadTextures().
	bool append_glyph(Uint32 codepoint);
	void reupload_textures();

	GFX::GfxContext *m_ctx = nullptr;

	std::vector<Uint8> m_font_data; // owns the TTF bytes; m_FontInfo points into this
	stbtt_fontinfo m_font_info{};
	F32 m_scale = 0.F;

	Ref<GFX::GfxTexture> m_curve_texture; // RGBA32F: 2 texels per curve (p0+p1, p2)
	Ref<GFX::GfxTexture> m_band_texture; // RGBA32U: band headers + curve index lists

	// CPU shadow copies of the GPU texture contents, sized to the reserved capacity.
	// New glyphs are appended at the cursors, then the full arrays are re-uploaded.
	std::vector<std::array<F32, 4>> m_curve_texels;
	std::vector<std::array<Uint32, 4>> m_band_texels;
	Uint32 m_curve_cursor = 0;
	Uint32 m_band_cursor = 0;
	Uint32 m_curve_capacity_texels = 0;
	Uint32 m_band_capacity_texels = 0;

	std::unordered_map<Uint32, GlyphInfo> m_glyphs;
	std::vector<SlugGlyphData> m_slug_glyphs;
	std::unordered_set<Uint32> m_missing_codepoints; // absent from the font; don't retry

	F32 m_line_height = 0.F;
	F32 m_ascent = 0.F;
	F32 m_descent = 0.F;
	F32 m_bake_size = 0.F;
};

} // namespace Aquila::UI::Text
