#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Text/Utf8.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"


namespace Aquila::UI::Text {

static constexpr Uint32 K_ATLAS_SIZE = 1024;
static constexpr Uint32 K_ATLAS_PADDING = 1;

GlyphInfo FontAtlas::build_glyph_info(const stbtt_fontinfo &font_info, int glyph_index, Uint32 glyph_id, F32 scale) {
	int advance_width = 0, left_side_bearing = 0;
	stbtt_GetGlyphHMetrics(&font_info, glyph_index, &advance_width, &left_side_bearing);

	int bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
	stbtt_GetGlyphBox(&font_info, glyph_index, &bx0, &by0, &bx1, &by1);

	GlyphInfo info{};
	info.glyph_id = glyph_id;
	info.size = { static_cast<F32>(bx1 - bx0) * scale, static_cast<F32>(by1 - by0) * scale };
	info.bearing = { static_cast<F32>(bx0) * scale, -static_cast<F32>(by1) * scale };
	info.advance = static_cast<F32>(advance_width) * scale;
	return info;
}

bool FontAtlas::append_glyph(Uint32 codepoint) {
	if (m_glyphs.find(codepoint) != m_glyphs.end()) {
		return false;
	}
	if (m_missing_codepoints.count(codepoint) != 0) {
		return false;
	}

	const int glyph_index = stbtt_FindGlyphIndex(&m_font_info, static_cast<int>(codepoint));
	if (glyph_index == 0) {
		m_missing_codepoints.insert(codepoint);
		return false;
	}

	m_glyphs[codepoint] = build_glyph_info(m_font_info, glyph_index, 0, m_scale);
	return true;
}

Uint64 FontAtlas::bitmap_key(Uint32 codepoint, Uint32 pixel_size) {
	return (static_cast<Uint64>(pixel_size) << 32) | static_cast<Uint64>(codepoint);
}

BitmapGlyph FontAtlas::rasterize_bitmap(Uint32 codepoint, Uint32 pixel_size) {
	BitmapGlyph glyph{};

	const F32 scale = stbtt_ScaleForMappingEmToPixels(&m_font_info, static_cast<F32>(pixel_size));
	const int glyph_index = stbtt_FindGlyphIndex(&m_font_info, static_cast<int>(codepoint));

	int advance_width = 0, left_side_bearing = 0;
	stbtt_GetGlyphHMetrics(&m_font_info, glyph_index, &advance_width, &left_side_bearing);
	glyph.advance = static_cast<F32>(advance_width) * scale;

	if (glyph_index == 0) {
		return glyph;
	}

	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
	stbtt_GetGlyphBitmapBox(&m_font_info, glyph_index, scale, scale, &x0, &y0, &x1, &y1);

	const int width = x1 - x0;
	const int height = y1 - y0;
	if (width <= 0 || height <= 0) {
		return glyph;
	}

	if (m_pack_x + static_cast<Uint32>(width) + K_ATLAS_PADDING > m_atlas_width) {
		m_pack_x = 0;
		m_pack_y += m_pack_row_height + K_ATLAS_PADDING;
		m_pack_row_height = 0;
	}
	if (m_pack_y + static_cast<Uint32>(height) > m_atlas_height) {
		AQUILA_LOG_ERROR("FontAtlas: bitmap atlas full (codepoint {} at {}px)", codepoint, pixel_size);
		return glyph;
	}

	std::vector<Uint8> coverage(static_cast<Usize>(width) * static_cast<Usize>(height));
	stbtt_MakeGlyphBitmap(&m_font_info, coverage.data(), width, height, width, scale, scale, glyph_index);

	for (int row = 0; row < height; ++row) {
		for (int col = 0; col < width; ++col) {
			const Uint8 alpha = coverage[static_cast<Usize>(row) * static_cast<Usize>(width) + col];
			const Uint32 dst = (m_pack_y + static_cast<Uint32>(row)) * m_atlas_width + (m_pack_x + static_cast<Uint32>(col));
			m_atlas_pixels[dst] = (static_cast<Uint32>(alpha) << 24) | 0x00FFFFFFu;
		}
	}

	const F32 atlas_w = static_cast<F32>(m_atlas_width);
	const F32 atlas_h = static_cast<F32>(m_atlas_height);
	glyph.size = { static_cast<F32>(width), static_cast<F32>(height) };
	glyph.bearing = { static_cast<F32>(x0), static_cast<F32>(y0) };
	glyph.uv_min = { static_cast<F32>(m_pack_x) / atlas_w, static_cast<F32>(m_pack_y) / atlas_h };
	glyph.uv_max = { static_cast<F32>(m_pack_x + static_cast<Uint32>(width)) / atlas_w,
					 static_cast<F32>(m_pack_y + static_cast<Uint32>(height)) / atlas_h };

	m_pack_x += static_cast<Uint32>(width) + K_ATLAS_PADDING;
	m_pack_row_height = std::max(m_pack_row_height, static_cast<Uint32>(height));
	m_atlas_dirty = true;
	return glyph;
}

void FontAtlas::upload_atlas() {
	m_ctx->upload_texture_data(*m_atlas_texture, m_atlas_pixels.data(), sizeof(Uint32) * m_atlas_pixels.size());
}

Unique<FontAtlas> FontAtlas::create(GFX::GfxContext &ctx, const Uint8 *ttf_data, Uint64 data_size, F32 pixel_height) {
	Unique<FontAtlas> atlas(new FontAtlas());
	atlas->m_ctx = &ctx;
	atlas->m_bake_size = pixel_height;

	atlas->m_font_data.assign(ttf_data, ttf_data + data_size);
	stbtt_InitFont(&atlas->m_font_info, atlas->m_font_data.data(),
				   stbtt_GetFontOffsetForIndex(atlas->m_font_data.data(), 0));

	atlas->m_scale = stbtt_ScaleForMappingEmToPixels(&atlas->m_font_info, pixel_height);

	int ascent = 0, descent = 0, line_gap = 0;
	stbtt_GetFontVMetrics(&atlas->m_font_info, &ascent, &descent, &line_gap);
	atlas->m_ascent = static_cast<F32>(ascent) * atlas->m_scale;
	atlas->m_descent = static_cast<F32>(descent) * atlas->m_scale;
	atlas->m_line_height = static_cast<F32>(ascent - descent + line_gap) * atlas->m_scale;

	atlas->m_atlas_width = K_ATLAS_SIZE;
	atlas->m_atlas_height = K_ATLAS_SIZE;
	atlas->m_atlas_pixels.assign(static_cast<Usize>(K_ATLAS_SIZE) * K_ATLAS_SIZE, 0u);

	atlas->m_atlas_texture = ctx.create_texture({
		.width = K_ATLAS_SIZE,
		.height = K_ATLAS_SIZE,
		.format = RHI::TextureFormat::RGBA8,
		.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst,
		.debug_name = "FontAtlas_Bitmap",
	});

	for (int i = 0; i < SharedConstants::FONT_GLYPH_COUNT; ++i) {
		atlas->append_glyph(static_cast<Uint32>(SharedConstants::FONT_FIRST_CODEPOINT + i));
	}

	return atlas;
}

Unique<FontAtlas> FontAtlas::create_from_file(GFX::GfxContext &ctx, const std::string &path, F32 pixel_height) {
	auto file = Platform::Filesystem::VirtualFileSystem::get()->open_file(path, AccessMode::Read, OpenMode::Binary);
	if (!file || !file->is_valid()) {
		AQUILA_LOG_ERROR("FontAtlas: cannot open '{}'", path);
		return nullptr;
	}

	const Int64 size = file->size();
	if (size <= 0) {
		return nullptr;
	}
	std::vector<Uint8> data(static_cast<Usize>(size));
	file->read(data.data(), static_cast<Usize>(size));
	return create(ctx, data.data(), static_cast<Uint64>(size), pixel_height);
}

const GlyphInfo *FontAtlas::get_glyph(Uint32 codepoint) const {
	auto it = m_glyphs.find(codepoint);
	return it != m_glyphs.end() ? &it->second : nullptr;
}

Vec2 FontAtlas::measure_text(std::string_view text, F32 font_size) const {
	const F32 render_size = (font_size > 0.F) ? font_size : m_bake_size;
	const F32 scale = (m_bake_size > 0.F && render_size > 0.F) ? (render_size / m_bake_size) : 1.F;

	F32 width = 0.F;
	for (Usize i = 0; i < text.size();) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::decode(text, i);
		i += (d.size > 0 ? d.size : 1u);
		if (const GlyphInfo *glyph = get_glyph(d.codepoint)) {
			width += glyph->advance * scale;
		}
	}
	return { width, m_line_height * scale };
}

void FontAtlas::ensure_glyphs(std::string_view text) {
	for (Usize i = 0; i < text.size();) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::decode(text, i);
		i += (d.size > 0 ? d.size : 1u);
		append_glyph(d.codepoint);
	}
}

void FontAtlas::ensure_bitmaps(std::string_view text, F32 font_size) {
	const F32 render_size = (font_size > 0.F) ? font_size : m_bake_size;
	const Uint32 pixel_size = static_cast<Uint32>(std::lround(render_size));
	if (pixel_size == 0) {
		return;
	}

	bool added = false;
	for (Usize i = 0; i < text.size();) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::decode(text, i);
		i += (d.size > 0 ? d.size : 1u);
		const Uint64 key = bitmap_key(d.codepoint, pixel_size);
		if (m_bitmaps.find(key) == m_bitmaps.end()) {
			m_bitmaps[key] = rasterize_bitmap(d.codepoint, pixel_size);
			added = true;
		}
	}

	if (added && m_atlas_dirty) {
		upload_atlas();
		m_atlas_dirty = false;
	}
}

const BitmapGlyph *FontAtlas::get_bitmap(Uint32 codepoint, F32 font_size) const {
	const F32 render_size = (font_size > 0.F) ? font_size : m_bake_size;
	const Uint32 pixel_size = static_cast<Uint32>(std::lround(render_size));
	auto it = m_bitmaps.find(bitmap_key(codepoint, pixel_size));
	return it != m_bitmaps.end() ? &it->second : nullptr;
}

} // namespace Aquila::UI::Text
