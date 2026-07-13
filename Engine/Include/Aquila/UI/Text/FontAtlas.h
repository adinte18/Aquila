#pragma once
#include "stb/stb_truetype.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/RHI/Backend/RHITypes.h"

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Aquila::UI::Text {

struct GlyphInfo {
	Uint32 glyph_id;
	Vec2 size;
	Vec2 bearing;
	F32 advance;
};

struct BitmapGlyph {
	Vec2 uv_min;
	Vec2 uv_max;
	Vec2 size;
	Vec2 bearing;
	F32 advance;
};

class FontAtlas {
  public:
	~FontAtlas() = default;
	AQUILA_NONCOPYABLE(FontAtlas);
	AQUILA_NONMOVEABLE(FontAtlas);

	static Unique<FontAtlas> create(GFX::GfxContext &ctx, const Uint8 *ttf_data, Uint64 data_size, F32 pixel_height);

	static Unique<FontAtlas> create_from_file(GFX::GfxContext &ctx, const std::string &path, F32 pixel_height);

	const GlyphInfo *get_glyph(Uint32 codepoint) const;

	[[nodiscard]] Vec2 measure_text(std::string_view text, F32 font_size) const;

	void ensure_glyphs(std::string_view text);

	void ensure_bitmaps(std::string_view text, F32 font_size);
	const BitmapGlyph *get_bitmap(Uint32 codepoint, F32 font_size) const;
	GFX::GfxTexture *get_atlas_texture() const { return m_atlas_texture.get(); }

	F32 get_line_height() const { return m_line_height; }
	F32 get_ascent() const { return m_ascent; }
	F32 get_descent() const { return m_descent; }
	F32 get_bake_size() const { return m_bake_size; }

  private:
	FontAtlas() = default;

	static GlyphInfo build_glyph_info(const stbtt_fontinfo &font_info, int glyph_index, Uint32 glyph_id, F32 scale);

	bool append_glyph(Uint32 codepoint);

	static Uint64 bitmap_key(Uint32 codepoint, Uint32 pixel_size);
	BitmapGlyph rasterize_bitmap(Uint32 codepoint, Uint32 pixel_size);
	void upload_atlas();

	GFX::GfxContext *m_ctx = nullptr;

	std::vector<Uint8> m_font_data;
	stbtt_fontinfo m_font_info{};
	F32 m_scale = 0.F;

	Ref<GFX::GfxTexture> m_atlas_texture;
	std::vector<Uint32> m_atlas_pixels;
	Uint32 m_atlas_width = 0;
	Uint32 m_atlas_height = 0;
	Uint32 m_pack_x = 0;
	Uint32 m_pack_y = 0;
	Uint32 m_pack_row_height = 0;
	bool m_atlas_dirty = false;

	std::unordered_map<Uint32, GlyphInfo> m_glyphs;
	std::unordered_map<Uint64, BitmapGlyph> m_bitmaps;
	std::unordered_set<Uint32> m_missing_codepoints;

	F32 m_line_height = 0.F;
	F32 m_ascent = 0.F;
	F32 m_descent = 0.F;
	F32 m_bake_size = 0.F;
};

} // namespace Aquila::UI::Text
