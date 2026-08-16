#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/UI/Text/FontAtlasBuilder.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Text/Utf8.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::UI::Text {

using namespace Internal;

void FontAtlas::build_glyph_curves(const stbtt_fontinfo &font_info, int glyph_index, F32 scale, GlyphBuild &out) {
	int bx0 = 0;
	int by0 = 0;
	int bx1 = 0;
	int by1 = 0;
	stbtt_GetGlyphBox(&font_info, glyph_index, &bx0, &by0, &bx1, &by1);

	out.em_min = { 0.F, 0.F };
	out.em_max = { static_cast<F32>(bx1 - bx0) * scale, static_cast<F32>(by1 - by0) * scale };

	auto to_local = [&](int x, int y) -> Vec2 {
		return { (static_cast<F32>(x) - static_cast<F32>(bx0)) * scale,
				 (static_cast<F32>(y) - static_cast<F32>(by0)) * scale };
	};

	auto add_curve = [&](Vec2 p0, Vec2 p1, Vec2 p2) {
		Math::Geometry::Bezier::QuadraticBezier curve{ .p0 = p0, .p1 = p1, .p2 = p2 };
		auto split = Math::Geometry::Bezier::split_at_y_extrema(curve);
		if (split.was_split) {
			out.curves.push_back(split.left);
			out.curves.push_back(split.right);
		} else {
			out.curves.push_back(curve);
		}
	};

	auto add_line = [&](Vec2 p0, Vec2 p2) { add_curve(p0, (p0 + p2) * 0.5f, p2); };

	stbtt_vertex *verts = nullptr;
	const int num_verts = stbtt_GetGlyphShape(&font_info, glyph_index, &verts);

	Vec2 cursor{};
	for (int vert = 0; vert < num_verts; ++vert) {
		switch (verts[vert].type) {
		case STBTT_vmove:
			cursor = to_local(verts[vert].x, verts[vert].y);
			break;

		case STBTT_vline: {
			Vec2 end = to_local(verts[vert].x, verts[vert].y);
			add_line(cursor, end);
			cursor = end;
			break;
		}

		case STBTT_vcurve: {
			Vec2 ctrl = to_local(verts[vert].cx, verts[vert].cy);
			Vec2 end = to_local(verts[vert].x, verts[vert].y);
			add_curve(cursor, ctrl, end);
			cursor = end;
			break;
		}

		case STBTT_vcubic: {
			Vec2 c1 = to_local(verts[vert].cx, verts[vert].cy);
			Vec2 c2 = to_local(verts[vert].cx1, verts[vert].cy1);
			Vec2 end = to_local(verts[vert].x, verts[vert].y);

			Vec2 mid = (cursor + 3.0f * c1 + 3.0f * c2 + end) * 0.125f;
			add_curve(cursor, (cursor + c1) * 0.5f, mid);
			add_curve(mid, (c2 + end) * 0.5f, end);
			cursor = end;
			break;
		}
		}
	}

	if (verts != nullptr) {
		stbtt_FreeShape(&font_info, verts);
	}
}

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

	GlyphBuild build;
	build_glyph_curves(m_font_info, glyph_index, m_scale, build);

	const Uint32 curve_need = static_cast<Uint32>(build.curves.size()) * SharedConstants::FONT_TEXELS_PER_CURVE;

	const F32 em_w = build.em_max.x - build.em_min.x;
	const F32 em_h = build.em_max.y - build.em_min.y;
	const F32 scale_x = (em_w > 0.01f) ? (static_cast<F32>(SharedConstants::FONT_BAND_COUNT) / em_w) : 0.F;
	const F32 scale_y = (em_h > 0.01f) ? (static_cast<F32>(SharedConstants::FONT_BAND_COUNT) / em_h) : 0.F;
	const F32 offset_x = -build.em_min.x * scale_x;
	const F32 offset_y = -build.em_min.y * scale_y;

	// The band header block (2 * FONT_BAND_COUNT texels) must not straddle a texture row.
	Uint32 band_start = m_band_cursor;
	{
		const Uint32 x = band_start % SharedConstants::FONT_TEX_WIDTH;
		if (x + (2 * SharedConstants::FONT_BAND_COUNT) > SharedConstants::FONT_TEX_WIDTH) {
			band_start += SharedConstants::FONT_TEX_WIDTH - x;
		}
	}

	const auto bands = bucket_curves_into_bands(build, m_curve_cursor, scale_x, scale_y, offset_x, offset_y);
	Uint32 total_h_idx = 0, total_v_idx = 0;
	for (const auto &hb : bands.horizontal) {
		total_h_idx += static_cast<Uint32>(hb.size());
	}
	for (const auto &vb : bands.vertical) {
		total_v_idx += static_cast<Uint32>(vb.size());
	}
	const Uint32 band_end = band_start + 2 * SharedConstants::FONT_BAND_COUNT + total_h_idx + total_v_idx;

	if (m_curve_cursor + curve_need > m_curve_capacity_texels || band_end > m_band_capacity_texels) {
		AQUILA_LOG_ERROR("FontAtlas: glyph atlas capacity exhausted (codepoint {}); glyph skipped", codepoint);
		m_missing_codepoints.insert(codepoint);
		return false;
	}

	const Uint32 glyph_id = static_cast<Uint32>(m_slug_glyphs.size());
	const Uint32 curve_base = m_curve_cursor;

	for (Uint32 c = 0; c < static_cast<Uint32>(build.curves.size()); ++c) {
		const auto &cv = build.curves[c];
		m_curve_texels[curve_base + c * 2] = { cv.p0.x, cv.p0.y, cv.p1.x, cv.p1.y };
		m_curve_texels[curve_base + c * 2 + 1] = { cv.p2.x, cv.p2.y, 0.F, 0.F };
	}
	m_curve_cursor = curve_base + curve_need;

	write_glyph_band_entries(m_band_texels, band_start, bands);
	m_band_cursor = band_end;

	m_slug_glyphs.push_back(SlugGlyphData{
		.glyph_loc_x = band_start % SharedConstants::FONT_TEX_WIDTH,
		.glyph_loc_y = band_start / SharedConstants::FONT_TEX_WIDTH,
		.band_max_x = SharedConstants::FONT_BAND_MAX,
		.band_max_y = SharedConstants::FONT_BAND_MAX,
		.band_transform = { scale_x, scale_y, offset_x, offset_y },
		.em_min = build.em_min,
		.em_max = build.em_max,
	});

	m_glyphs[codepoint] = build_glyph_info(m_font_info, glyph_index, glyph_id, m_scale);
	return true;
}

void FontAtlas::reupload_textures() {
	m_ctx->upload_texture_data(*m_curve_texture, m_curve_texels.data(), sizeof(F32) * 4 * m_curve_texels.size());
	m_ctx->upload_texture_data(*m_band_texture, m_band_texels.data(), sizeof(Uint32) * 4 * m_band_texels.size());
}

Unique<FontAtlas> FontAtlas::create(GFX::GfxContext &ctx, const Uint8 *ttf_data, Uint64 data_size, F32 pixel_height) {
	Unique<FontAtlas> atlas(new FontAtlas());
	atlas->m_ctx = &ctx;
	atlas->m_bake_size = pixel_height;

	atlas->m_font_data.assign(ttf_data, ttf_data + data_size);
	stbtt_InitFont(&atlas->m_font_info, atlas->m_font_data.data(),
				   stbtt_GetFontOffsetForIndex(atlas->m_font_data.data(), 0));

	// Em mapping ("M is N px"), matching CSS/Godot font-size semantics. ScaleForPixelHeight
	// would instead squeeze the full ascent-descent span into pixelHeight, rendering glyphs
	// noticeably smaller than the requested size.
	atlas->m_scale = stbtt_ScaleForMappingEmToPixels(&atlas->m_font_info, pixel_height);

	int ascent = 0, descent = 0, line_gap = 0;
	stbtt_GetFontVMetrics(&atlas->m_font_info, &ascent, &descent, &line_gap);
	atlas->m_ascent = static_cast<F32>(ascent) * atlas->m_scale;
	atlas->m_descent = static_cast<F32>(descent) * atlas->m_scale;
	atlas->m_line_height = static_cast<F32>(ascent - descent + line_gap) * atlas->m_scale;

	atlas->m_curve_capacity_texels = SharedConstants::FONT_CURVE_RESERVE_ROWS * SharedConstants::FONT_TEX_WIDTH;
	atlas->m_band_capacity_texels = SharedConstants::FONT_BAND_RESERVE_ROWS * SharedConstants::FONT_TEX_WIDTH;
	atlas->m_curve_texels.assign(atlas->m_curve_capacity_texels, { 0.F, 0.F, 0.F, 0.F });
	atlas->m_band_texels.assign(atlas->m_band_capacity_texels, { 0u, 0u, 0u, 0u });

	atlas->m_curve_texture = ctx.create_texture({
		.width = SharedConstants::FONT_TEX_WIDTH,
		.height = SharedConstants::FONT_CURVE_RESERVE_ROWS,
		.format = RHI::TextureFormat::RGBA32F,
		.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst,
		.debug_name = "FontAtlas_Curves",
	});
	atlas->m_band_texture = ctx.create_texture({
		.width = SharedConstants::FONT_TEX_WIDTH,
		.height = SharedConstants::FONT_BAND_RESERVE_ROWS,
		.format = RHI::TextureFormat::RGBA32U,
		.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst,
		.sampler = RHI::SamplerDesc::point_sample(),
		.debug_name = "FontAtlas_Bands",
	});

	for (int i = 0; i < SharedConstants::FONT_GLYPH_COUNT; ++i) {
		atlas->append_glyph(static_cast<Uint32>(SharedConstants::FONT_FIRST_CODEPOINT + i));
	}
	atlas->reupload_textures();

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

const SlugGlyphData *FontAtlas::get_slug_data(Uint32 glyph_id) const {
	if (glyph_id < static_cast<Uint32>(m_slug_glyphs.size())) {
		return &m_slug_glyphs[glyph_id];
	}
	return nullptr;
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
	bool added = false;
	for (Usize i = 0; i < text.size();) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::decode(text, i);
		i += (d.size > 0 ? d.size : 1u);
		added |= append_glyph(d.codepoint);
	}
	if (added) {
		reupload_textures();
	}
}

} // namespace Aquila::UI::Text
