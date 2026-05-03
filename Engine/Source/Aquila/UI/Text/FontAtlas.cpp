#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/RHI/Backend/RHITypes.h"

#include <fstream>
#include <vector>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"
#pragma clang diagnostic pop

namespace Aquila::UI::Text {

static constexpr int k_AtlasWidth = 512;
static constexpr int k_AtlasHeight = 512;
static constexpr int k_FirstCodepoint = 32;
static constexpr int k_GlyphCount = 96; // ASCII 32–127

FontAtlas::~FontAtlas() {
	if (m_Texture) {
		m_Ctx->DestroyImmediateTexture(*m_Texture);
	}
}

Unique<FontAtlas> FontAtlas::Create(GFX::GfxContext &ctx, const uint8 *ttfData, uint64 dataSize, float pixelHeight) {
	AQUILA_UNUSED(dataSize);

	Unique<FontAtlas> atlas(new FontAtlas());

	stbtt_fontinfo fontInfo{};
	stbtt_InitFont(&fontInfo, ttfData, stbtt_GetFontOffsetForIndex(ttfData, 0));

	const float scale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

	int ascent = 0;
	int descent = 0;
	int lineGap = 0;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

	atlas->m_Ascent = static_cast<float>(ascent) * scale;
	atlas->m_Descent = static_cast<float>(descent) * scale;
	atlas->m_LineHeight = static_cast<float>(ascent - descent + lineGap) * scale;

	std::vector<stbtt_packedchar> packedChars(k_GlyphCount);
	std::vector<uint8> bitmap(k_AtlasWidth * k_AtlasHeight, 0);

	stbtt_pack_context packCtx{};
	stbtt_PackBegin(&packCtx, bitmap.data(), k_AtlasWidth, k_AtlasHeight, 0, 1, nullptr);
	stbtt_PackSetOversampling(&packCtx, 2, 2);
	stbtt_PackFontRange(&packCtx, ttfData, 0, pixelHeight, k_FirstCodepoint, k_GlyphCount, packedChars.data());
	stbtt_PackEnd(&packCtx);

	for (int i = 0; i < k_GlyphCount; ++i) {
		const stbtt_packedchar &pc = packedChars[i];
		const auto codepoint = static_cast<uint32>(k_FirstCodepoint + i);

		GlyphInfo info{};
		f32 x = 0.f;
		f32 y = 0.f;
		stbtt_aligned_quad q{};
		stbtt_GetPackedQuad(packedChars.data(), k_AtlasWidth, k_AtlasHeight, i, &x, &y, &q, 0);

		info.uvMin = { q.s0, q.t0 };
		info.uvMax = { q.s1, q.t1 };
		info.size = { q.x1 - q.x0, q.y1 - q.y0 };
		info.bearing = { q.x0, q.y0 }; // offset from cursor to top-left of glyph
		info.advance = packedChars[i].xadvance;

		atlas->m_Glyphs[codepoint] = info;
	}

	RHI::TextureDesc desc{};
	desc.width = k_AtlasWidth;
	desc.height = k_AtlasHeight;
	desc.format = RHI::TextureFormat::R8;
	desc.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst;
	desc.sampler = RHI::SamplerDesc::RenderTarget();
	desc.debugName = "FontAtlas";

	atlas->m_Ctx = &ctx;
	atlas->m_Texture = ctx.CreateTexture(desc);

	ctx.UploadTextureData(*atlas->m_Texture, bitmap.data(), static_cast<uint64>(k_AtlasWidth * k_AtlasHeight));

	return atlas;
}

Unique<FontAtlas> FontAtlas::CreateFromFile(GFX::GfxContext &ctx, const std::string &path, float pixelHeight) {
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		return nullptr;
	}

	const auto size = static_cast<uint64>(file.tellg());
	file.seekg(0);

	std::vector<uint8> data(size);
	file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(size));

	return Create(ctx, data.data(), size, pixelHeight);
}

const GlyphInfo *FontAtlas::GetGlyph(uint32 codepoint) const {
	auto it = m_Glyphs.find(codepoint);
	return it != m_Glyphs.end() ? &it->second : nullptr;
}

} // namespace Aquila::UI::Text
