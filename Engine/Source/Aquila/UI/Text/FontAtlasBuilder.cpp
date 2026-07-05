#include "Aquila/UI/Text/FontAtlasBuilder.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Math/Math.h"

#include <algorithm>

namespace Aquila::UI::Text::Internal {

std::vector<std::array<F32, 4>> build_curve_texture_data(const std::vector<GlyphBuild> &builds,
														 const std::vector<Uint32> &glyph_curve_start,
														 Uint32 curve_texel_count, Uint32 count) {
	const Uint32 tex_h = (curve_texel_count + SharedConstants::FONT_TEX_WIDTH - 1) / SharedConstants::FONT_TEX_WIDTH;
	std::vector<std::array<F32, 4>> data(tex_h * SharedConstants::FONT_TEX_WIDTH, { 0.F, 0.F, 0.F, 0.F });

	for (Uint32 i = 0; i < count; ++i) {
		const Uint32 base = glyph_curve_start[i];
		const auto &curves = builds[i].curves;

		for (Uint32 c = 0; c < static_cast<Uint32>(curves.size()); ++c) {
			const auto &cv = curves[c];
			data[base + c * 2] = { cv.p0.x, cv.p0.y, cv.p1.x, cv.p1.y };
			data[base + c * 2 + 1] = { cv.p2.x, cv.p2.y, 0.F, 0.F };
		}
	}
	return data;
}

GlyphBandData bucket_curves_into_bands(const GlyphBuild &build, Uint32 curve_base, F32 scale_x, F32 scale_y,
									   F32 offset_x, F32 offset_y) {
	GlyphBandData result{};

	for (Uint32 c = 0; c < static_cast<Uint32>(build.curves.size()); ++c) {
		const auto &cv = build.curves[c];
		const Uint32 abs_texel = curve_base + c * SharedConstants::FONT_TEXELS_PER_CURVE;
		const Uint32 tx = abs_texel % SharedConstants::FONT_TEX_WIDTH;
		const Uint32 ty = abs_texel / SharedConstants::FONT_TEX_WIDTH;

		const auto bounds = Math::Bezier::compute_bounds(cv);
		const F32 x_min = bounds.min.x, x_max = bounds.max.x;
		const F32 y_min = bounds.min.y, y_max = bounds.max.y;

		const int by_min = Math::max(0, static_cast<int>(Math::floor(y_min * scale_y + offset_y)));
		const int by_max = Math::min(static_cast<int>(SharedConstants::FONT_BAND_MAX),
									 static_cast<int>(Math::floor(y_max * scale_y + offset_y)));
		for (int b = by_min; b <= by_max; ++b) {
			result.horizontal[static_cast<Uint32>(b)].push_back({ tx, ty, x_max });
		}

		const int bx_min = Math::max(0, static_cast<int>(Math::floor(x_min * scale_x + offset_x)));
		const int bx_max = Math::min(static_cast<int>(SharedConstants::FONT_BAND_MAX),
									 static_cast<int>(Math::floor(x_max * scale_x + offset_x)));
		for (int b = bx_min; b <= bx_max; ++b) {
			result.vertical[static_cast<Uint32>(b)].push_back({ tx, ty, y_max });
		}
	}

	auto desc_sort = [](const BandEntry &a, const BandEntry &b) { return a.sort_key > b.sort_key; };
	for (auto &hb : result.horizontal) {
		std::sort(hb.begin(), hb.end(), desc_sort);
	}
	for (auto &vb : result.vertical) {
		std::sort(vb.begin(), vb.end(), desc_sort);
	}

	return result;
}

void write_glyph_band_entries(std::vector<std::array<Uint32, 4>> &band_tex_data, Uint32 band_start,
							  const GlyphBandData &bands) {
	auto ensure_size = [&](Uint32 needed) {
		if (band_tex_data.size() < needed) {
			band_tex_data.resize(needed, { 0u, 0u, 0u, 0u });
		}
	};

	Uint32 total_h_idx = 0;
	for (const auto &hb : bands.horizontal) {
		total_h_idx += static_cast<Uint32>(hb.size());
	}
	Uint32 total_v_idx = 0;
	for (const auto &vb : bands.vertical) {
		total_v_idx += static_cast<Uint32>(vb.size());
	}

	const Uint32 header_slots = 2 * SharedConstants::FONT_BAND_COUNT;
	ensure_size(band_start + header_slots + total_h_idx + total_v_idx);

	const Uint32 h_idx_base = band_start + header_slots;
	Uint32 h_idx_cursor = h_idx_base;
	for (Uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		const Uint32 count = static_cast<Uint32>(bands.horizontal[b].size());
		const Uint32 offset = (count > 0) ? (h_idx_cursor - band_start) : 0u;
		band_tex_data[band_start + b] = { count, offset, 0u, 0u };
		h_idx_cursor += count;
	}

	const Uint32 v_idx_base = h_idx_base + total_h_idx;
	Uint32 v_idx_cursor = v_idx_base;
	for (Uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		const Uint32 count = static_cast<Uint32>(bands.vertical[b].size());
		const Uint32 offset = (count > 0) ? (v_idx_cursor - band_start) : 0u;
		band_tex_data[band_start + SharedConstants::FONT_BAND_COUNT + b] = { count, offset, 0u, 0u };
		v_idx_cursor += count;
	}

	Uint32 abs_idx = h_idx_base;
	for (Uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		for (const auto &e : bands.horizontal[b]) {
			band_tex_data[abs_idx++] = { e.texel_x, e.texel_y, 0u, 0u };
		}
	}

	abs_idx = v_idx_base;
	for (Uint32 b = 0; b < SharedConstants::FONT_BAND_COUNT; ++b) {
		for (const auto &e : bands.vertical[b]) {
			band_tex_data[abs_idx++] = { e.texel_x, e.texel_y, 0u, 0u };
		}
	}
}

} // namespace Aquila::UI::Text::Internal
