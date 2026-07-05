#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/UI/Text/FontAtlasBuilder.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Text/Utf8.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::UI::Text {

using namespace Internal;

void FontAtlas::BuildGlyphCurves(const stbtt_fontinfo &fontInfo, int glyphIndex, f32 scale, GlyphBuild &out) {
	int bx0 = 0;
	int by0 = 0;
	int bx1 = 0;
	int by1 = 0;
	stbtt_GetGlyphBox(&fontInfo, glyphIndex, &bx0, &by0, &bx1, &by1);

	out.emMin = { 0.f, 0.f };
	out.emMax = { static_cast<f32>(bx1 - bx0) * scale, static_cast<f32>(by1 - by0) * scale };

	auto toLocal = [&](int x, int y) -> vec2 {
		return { (static_cast<f32>(x) - static_cast<f32>(bx0)) * scale,
				 (static_cast<f32>(y) - static_cast<f32>(by0)) * scale };
	};

	auto addCurve = [&](vec2 p0, vec2 p1, vec2 p2) {
		Math::Bezier::QuadraticBezier curve{ .p0 = p0, .p1 = p1, .p2 = p2 };
		auto split = Math::Bezier::SplitAtYExtrema(curve);
		if (split.wasSplit) {
			out.curves.push_back(split.left);
			out.curves.push_back(split.right);
		} else {
			out.curves.push_back(curve);
		}
	};

	auto addLine = [&](vec2 p0, vec2 p2) { addCurve(p0, (p0 + p2) * 0.5f, p2); };

	stbtt_vertex *verts = nullptr;
	const int numVerts = stbtt_GetGlyphShape(&fontInfo, glyphIndex, &verts);

	vec2 cursor{};
	for (int vert = 0; vert < numVerts; ++vert) {
		switch (verts[vert].type) {
		case STBTT_vmove:
			cursor = toLocal(verts[vert].x, verts[vert].y);
			break;

		case STBTT_vline: {
			vec2 end = toLocal(verts[vert].x, verts[vert].y);
			addLine(cursor, end);
			cursor = end;
			break;
		}

		case STBTT_vcurve: {
			vec2 ctrl = toLocal(verts[vert].cx, verts[vert].cy);
			vec2 end = toLocal(verts[vert].x, verts[vert].y);
			addCurve(cursor, ctrl, end);
			cursor = end;
			break;
		}

		case STBTT_vcubic: {
			vec2 c1 = toLocal(verts[vert].cx, verts[vert].cy);
			vec2 c2 = toLocal(verts[vert].cx1, verts[vert].cy1);
			vec2 end = toLocal(verts[vert].x, verts[vert].y);

			vec2 mid = (cursor + 3.0f * c1 + 3.0f * c2 + end) * 0.125f;
			addCurve(cursor, (cursor + c1) * 0.5f, mid);
			addCurve(mid, (c2 + end) * 0.5f, end);
			cursor = end;
			break;
		}
		}
	}

	if (verts != nullptr) {
		stbtt_FreeShape(&fontInfo, verts);
	}
}

GlyphInfo FontAtlas::BuildGlyphInfo(const stbtt_fontinfo &fontInfo, int glyphIndex, uint32 glyphID, f32 scale) {
	int advanceWidth = 0, leftSideBearing = 0;
	stbtt_GetGlyphHMetrics(&fontInfo, glyphIndex, &advanceWidth, &leftSideBearing);

	int bx0 = 0, by0 = 0, bx1 = 0, by1 = 0;
	stbtt_GetGlyphBox(&fontInfo, glyphIndex, &bx0, &by0, &bx1, &by1);

	GlyphInfo info{};
	info.glyphID = glyphID;
	info.size = { static_cast<f32>(bx1 - bx0) * scale, static_cast<f32>(by1 - by0) * scale };
	info.bearing = { static_cast<f32>(bx0) * scale, -static_cast<f32>(by1) * scale };
	info.advance = static_cast<f32>(advanceWidth) * scale;
	return info;
}

bool FontAtlas::AppendGlyph(uint32 codepoint) {
	if (m_Glyphs.find(codepoint) != m_Glyphs.end()) {
		return false;
	}
	if (m_MissingCodepoints.count(codepoint) != 0) {
		return false;
	}

	const int glyphIndex = stbtt_FindGlyphIndex(&m_FontInfo, static_cast<int>(codepoint));
	if (glyphIndex == 0) {
		m_MissingCodepoints.insert(codepoint);
		return false;
	}

	GlyphBuild build;
	BuildGlyphCurves(m_FontInfo, glyphIndex, m_Scale, build);

	const uint32 curveNeed = static_cast<uint32>(build.curves.size()) * SharedConstants::FONT_TEXELS_PER_CURVE;

	const f32 emW = build.emMax.x - build.emMin.x;
	const f32 emH = build.emMax.y - build.emMin.y;
	const f32 scaleX = (emW > 0.01f) ? (static_cast<f32>(SharedConstants::FONT_BAND_COUNT) / emW) : 0.f;
	const f32 scaleY = (emH > 0.01f) ? (static_cast<f32>(SharedConstants::FONT_BAND_COUNT) / emH) : 0.f;
	const f32 offsetX = -build.emMin.x * scaleX;
	const f32 offsetY = -build.emMin.y * scaleY;

	// The band header block (2 * FONT_BAND_COUNT texels) must not straddle a texture row.
	uint32 bandStart = m_BandCursor;
	{
		const uint32 x = bandStart % SharedConstants::FONT_TEX_WIDTH;
		if (x + (2 * SharedConstants::FONT_BAND_COUNT) > SharedConstants::FONT_TEX_WIDTH) {
			bandStart += SharedConstants::FONT_TEX_WIDTH - x;
		}
	}

	const auto bands = BucketCurvesIntoBands(build, m_CurveCursor, scaleX, scaleY, offsetX, offsetY);
	uint32 totalHIdx = 0, totalVIdx = 0;
	for (const auto &hb : bands.horizontal) {
		totalHIdx += static_cast<uint32>(hb.size());
	}
	for (const auto &vb : bands.vertical) {
		totalVIdx += static_cast<uint32>(vb.size());
	}
	const uint32 bandEnd = bandStart + 2 * SharedConstants::FONT_BAND_COUNT + totalHIdx + totalVIdx;

	if (m_CurveCursor + curveNeed > m_CurveCapacityTexels || bandEnd > m_BandCapacityTexels) {
		AQUILA_LOG_ERROR("FontAtlas: glyph atlas capacity exhausted (codepoint {}); glyph skipped", codepoint);
		m_MissingCodepoints.insert(codepoint);
		return false;
	}

	const uint32 glyphID = static_cast<uint32>(m_SlugGlyphs.size());
	const uint32 curveBase = m_CurveCursor;

	for (uint32 c = 0; c < static_cast<uint32>(build.curves.size()); ++c) {
		const auto &cv = build.curves[c];
		m_CurveTexels[curveBase + c * 2] = { cv.p0.x, cv.p0.y, cv.p1.x, cv.p1.y };
		m_CurveTexels[curveBase + c * 2 + 1] = { cv.p2.x, cv.p2.y, 0.f, 0.f };
	}
	m_CurveCursor = curveBase + curveNeed;

	WriteGlyphBandEntries(m_BandTexels, bandStart, bands);
	m_BandCursor = bandEnd;

	m_SlugGlyphs.push_back(SlugGlyphData{
		.glyphLocX = bandStart % SharedConstants::FONT_TEX_WIDTH,
		.glyphLocY = bandStart / SharedConstants::FONT_TEX_WIDTH,
		.bandMaxX = SharedConstants::FONT_BAND_MAX,
		.bandMaxY = SharedConstants::FONT_BAND_MAX,
		.bandTransform = { scaleX, scaleY, offsetX, offsetY },
		.emMin = build.emMin,
		.emMax = build.emMax,
	});

	m_Glyphs[codepoint] = BuildGlyphInfo(m_FontInfo, glyphIndex, glyphID, m_Scale);
	return true;
}

void FontAtlas::ReuploadTextures() {
	m_Ctx->UploadTextureData(*m_CurveTexture, m_CurveTexels.data(), sizeof(f32) * 4 * m_CurveTexels.size());
	m_Ctx->UploadTextureData(*m_BandTexture, m_BandTexels.data(), sizeof(uint32) * 4 * m_BandTexels.size());
}

Unique<FontAtlas> FontAtlas::Create(GFX::GfxContext &ctx, const uint8 *ttfData, uint64 dataSize, f32 pixelHeight) {
	Unique<FontAtlas> atlas(new FontAtlas());
	atlas->m_Ctx = &ctx;
	atlas->m_BakeSize = pixelHeight;

	atlas->m_FontData.assign(ttfData, ttfData + dataSize);
	stbtt_InitFont(&atlas->m_FontInfo, atlas->m_FontData.data(),
				   stbtt_GetFontOffsetForIndex(atlas->m_FontData.data(), 0));

	// Em mapping ("M is N px"), matching CSS/Godot font-size semantics. ScaleForPixelHeight
	// would instead squeeze the full ascent-descent span into pixelHeight, rendering glyphs
	// noticeably smaller than the requested size.
	atlas->m_Scale = stbtt_ScaleForMappingEmToPixels(&atlas->m_FontInfo, pixelHeight);

	int ascent = 0, descent = 0, lineGap = 0;
	stbtt_GetFontVMetrics(&atlas->m_FontInfo, &ascent, &descent, &lineGap);
	atlas->m_Ascent = static_cast<f32>(ascent) * atlas->m_Scale;
	atlas->m_Descent = static_cast<f32>(descent) * atlas->m_Scale;
	atlas->m_LineHeight = static_cast<f32>(ascent - descent + lineGap) * atlas->m_Scale;

	atlas->m_CurveCapacityTexels = SharedConstants::FONT_CURVE_RESERVE_ROWS * SharedConstants::FONT_TEX_WIDTH;
	atlas->m_BandCapacityTexels = SharedConstants::FONT_BAND_RESERVE_ROWS * SharedConstants::FONT_TEX_WIDTH;
	atlas->m_CurveTexels.assign(atlas->m_CurveCapacityTexels, { 0.f, 0.f, 0.f, 0.f });
	atlas->m_BandTexels.assign(atlas->m_BandCapacityTexels, { 0u, 0u, 0u, 0u });

	atlas->m_CurveTexture = ctx.CreateTexture({
		.width = SharedConstants::FONT_TEX_WIDTH,
		.height = SharedConstants::FONT_CURVE_RESERVE_ROWS,
		.format = RHI::TextureFormat::RGBA32F,
		.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst,
		.debugName = "FontAtlas_Curves",
	});
	atlas->m_BandTexture = ctx.CreateTexture({
		.width = SharedConstants::FONT_TEX_WIDTH,
		.height = SharedConstants::FONT_BAND_RESERVE_ROWS,
		.format = RHI::TextureFormat::RGBA32U,
		.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst,
		.sampler = RHI::SamplerDesc::PointSample(),
		.debugName = "FontAtlas_Bands",
	});

	for (int i = 0; i < SharedConstants::FONT_GLYPH_COUNT; ++i) {
		atlas->AppendGlyph(static_cast<uint32>(SharedConstants::FONT_FIRST_CODEPOINT + i));
	}
	atlas->ReuploadTextures();

	return atlas;
}

Unique<FontAtlas> FontAtlas::CreateFromFile(GFX::GfxContext &ctx, const std::string &path, f32 pixelHeight) {
	auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(path, AccessMode::Read, OpenMode::Binary);
	if (!file || !file->IsValid()) {
		AQUILA_LOG_ERROR("FontAtlas: cannot open '{}'", path);
		return nullptr;
	}

	const int64 size = file->Size();
	if (size <= 0) {
		return nullptr;
	}
	std::vector<uint8> data(static_cast<usize>(size));
	file->Read(data.data(), static_cast<usize>(size));
	return Create(ctx, data.data(), static_cast<uint64>(size), pixelHeight);
}

const GlyphInfo *FontAtlas::GetGlyph(uint32 codepoint) const {
	auto it = m_Glyphs.find(codepoint);
	return it != m_Glyphs.end() ? &it->second : nullptr;
}

const SlugGlyphData *FontAtlas::GetSlugData(uint32 glyphID) const {
	if (glyphID < static_cast<uint32>(m_SlugGlyphs.size())) {
		return &m_SlugGlyphs[glyphID];
	}
	return nullptr;
}

void FontAtlas::EnsureGlyphs(std::string_view text) {
	bool added = false;
	for (usize i = 0; i < text.size();) {
		const Foundation::Utf8::Decoded d = Foundation::Utf8::Decode(text, i);
		i += (d.size > 0 ? d.size : 1u);
		added |= AppendGlyph(d.codepoint);
	}
	if (added) {
		ReuploadTextures();
	}
}

} // namespace Aquila::UI::Text
