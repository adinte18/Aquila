#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include <array>

namespace Aquila::Graphics {

struct QuadVertex {
	vec3 position;
	uint32 glyphID = 0;
	vec4 color;
	vec2 uv;
	vec2 size;
	vec4 radius;
	float borderWidth;
	float _pad1[3]{};
	vec4 borderColor;
};

struct QuadPushConstants {
	mat4 viewProjection;
};

struct RectSpec {
	vec2 position = { 0.F, 0.F };
	vec2 size = { 1.F, 1.F };
	vec4 color = { 1.F, 1.F, 1.F, 1.F };
	float rotation = 0.F;
	float depth = 0.F;
	vec4 radius = { 0.F, 0.F, 0.F, 0.F };
	float borderWidth = 0.F;
	vec4 borderColor = { 0.F, 0.F, 0.F, 0.F };
};

struct SpriteSpec {
	vec2 position = { 0.F, 0.F };
	vec2 size = { 1.F, 1.F };
	vec4 tint = { 1.F, 1.F, 1.F, 1.F };
	float rotation = 0.F;
	float depth = 0.F;
	GFX::GfxTexture *texture = nullptr;
	vec2 uvMin = { 0.F, 0.F };
	vec2 uvMax = { 1.F, 1.F };
};

// Vertex format used exclusively by the text (Slug) pipeline.
// banding and glyphData are declared nointerpolation in the shader;
// all 4 vertices of a glyph quad carry the same values for those fields.
struct TextVertex {
	vec3 position; // screen-space XYZ (Z = depth)
	float _pad0{};
	vec4 color;
	vec2 texcoord; // em-space coordinate (interpolated)
	float texLoc{}; // bit_cast<float>(glyphLocX | (glyphLocY << 16))
	float bandMax{}; // bit_cast<float>((bandMaxX & 0xFF) | (bandMaxY << 8))
	vec4 banding; // (scaleX, scaleY, offsetX, offsetY) nointerpolation
};
static_assert(sizeof(TextVertex) == 64);

struct ShadowSpec {
	vec2 position = { 0.F, 0.F }; // top-left of the expanded shadow quad
	vec2 size = { 1.F, 1.F }; // shadow quad size = widgetSize + 2*(blur+spread)
	vec4 color = { 0.F, 0.F, 0.F, 0.75F };
	vec2 offset = { 0.F, 0.F }; // CSS shadow-offset (x, y)
	vec2 originalHalfSize = { 0.F, 0.F }; // widgetHalfSize + spread (SDF box)
	vec4 radius = { 0.F, 0.F, 0.F, 0.F };
	float blur = 0.F;
	float depth = 0.F;
};

struct GlyphSpec {
	vec2 position = { 0.F, 0.F }; // top-left screen position
	vec2 size = { 1.F, 1.F }; // screen size
	vec4 color = { 1.F, 1.F, 1.F, 1.F };
	float depth = 0.F;
	// Slug per-glyph data (constant across all 4 vertices).
	uint32 glyphLocX = 0;
	uint32 glyphLocY = 0;
	uint32 bandMaxX = 15;
	uint32 bandMaxY = 15;
	vec4 banding = { 0.F, 0.F, 0.F, 0.F };
	// Em-space extents for computing per-corner texcoords.
	vec2 emMin = { 0.F, 0.F };
	vec2 emMax = { 1.F, 1.F };
	GFX::GfxTexture *curveTexture = nullptr;
	GFX::GfxTexture *bandTexture = nullptr;
	bool flipY = false;
};

class QuadBatcher {
  public:
	explicit QuadBatcher(GFX::GfxContext &ctx);
	~QuadBatcher() = default;
	AQUILA_NONCOPYABLE(QuadBatcher);
	AQUILA_NONMOVEABLE(QuadBatcher);

	void Begin(GFX::GfxCommandList &cmd, RHI::TextureFormat colorFormat, RHI::SampleCount sampleCount,
			   const mat4 &viewProjection, RHI::TextureFormat depthFormat = RHI::TextureFormat::None);
	void Flush();
	void End();

	void BeginCapture();
	void ExecuteReplay(GFX::GfxCommandList &cmd);
	void SetScissor(GFX::GfxCommandList &cmd, int32 x, int32 y, uint32 w, uint32 h);

	void DrawRect(const RectSpec &spec);
	void DrawShadow(const ShadowSpec &spec);
	void DrawSprite(const SpriteSpec &spec);
	void DrawGlyph(const GlyphSpec &spec);

	void ResetStats() { m_Stats = {}; }
	[[nodiscard]] uint32 GetDrawCallCount() const { return m_Stats.drawCalls; }
	[[nodiscard]] uint32 GetQuadCount() const { return m_Stats.quadCount; }

	GFX::GfxPipeline &GetOrCreateFlatPipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
											  RHI::TextureFormat depthFormat);
	GFX::GfxPipeline &GetOrCreateTexturePipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
												 RHI::TextureFormat depthFormat);
	GFX::GfxPipeline &GetOrCreateGUIPipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
											 RHI::TextureFormat depthFormat);
	GFX::GfxPipeline &GetOrCreateTextPipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
											  RHI::TextureFormat depthFormat);
	GFX::GfxPipeline &GetOrCreateShadowPipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
												RHI::TextureFormat depthFormat);

  private:
	struct Stats {
		uint32 drawCalls = 0;
		uint32 quadCount = 0;
	};

	struct PipelineKey {
		RHI::TextureFormat colorFormat;
		RHI::SampleCount sampleCount;
		RHI::TextureFormat depthFormat;
		bool operator==(const PipelineKey &o) const {
			return colorFormat == o.colorFormat && sampleCount == o.sampleCount && depthFormat == o.depthFormat;
		}
	};
	struct PipelineKeyHash {
		size_t operator()(const PipelineKey &k) const {
			size_t h = std::hash<int>{}(static_cast<int>(k.colorFormat));
			h ^= std::hash<int>{}(static_cast<int>(k.sampleCount)) << 16;
			h ^= std::hash<int>{}(static_cast<int>(k.depthFormat)) << 24;
			return h;
		}
	};

	enum class BatchType { Flat, GUI, Texture, Text, Shadow };

	struct ReplayEntry {
		GFX::GfxPipeline *pipeline;
		GFX::GfxDescriptorSet *descSet; // null for pipelines with no descriptor set
		bool isTextBuffer; // selects text VB vs quad VB
		uint32 indexCount;
		int32 vertexOffset;
		bool isScissor = false;
		int32 scissorX = 0;
		int32 scissorY = 0;
		uint32 scissorW = 0;
		uint32 scissorH = 0;
	};

	void StartBatch();
	[[nodiscard]] mat4 BuildQuadTransform(vec2 position, vec2 size, float rotation, float depth) const;
	GFX::GfxDescriptorSet &GetOrCreateTextureSet(GFX::GfxTexture &texture);
	GFX::GfxDescriptorSet &GetOrCreateTextDataSet(GFX::GfxTexture &curveTexture, GFX::GfxTexture &bandTexture);

	GFX::GfxContext &m_Ctx;

	// Per-frame ring buffers — cycle between two slots to avoid GPU/CPU sync stalls.
	static constexpr uint32 kRingSize = SharedConstants::MAX_FRAMES_IN_FLIGHT;
	std::array<Ref<GFX::GfxBuffer>, kRingSize> m_VertexBuffers;
	std::array<Ref<GFX::GfxBuffer>, kRingSize> m_TextVertexBuffers;
	QuadVertex *m_MappedQuadBases[kRingSize] = {};
	TextVertex *m_MappedTextBases[kRingSize] = {};
	GFX::GfxBuffer *m_ActiveVertexBuffer = nullptr;
	GFX::GfxBuffer *m_ActiveTextVertexBuffer = nullptr;
	// Direct write pointers into the currently active mapped buffer — no intermediate vector.
	QuadVertex *m_QuadWritePtr = nullptr;
	TextVertex *m_TextWritePtr = nullptr;
	uint32 m_FrameCounter = 0;

	Ref<GFX::GfxBuffer> m_IndexBuffer;
	Ref<GFX::GfxDescriptorSetLayout> m_TextureLayout;
	Ref<GFX::GfxDescriptorSetLayout> m_TextDataLayout;
	std::unordered_map<GFX::GfxTexture *, Ref<GFX::GfxDescriptorSet>> m_TextureSetCache;
	std::unordered_map<GFX::GfxTexture *, Ref<GFX::GfxDescriptorSet>> m_TextDataSetCache;

	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_FlatPipelines;
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_TexturePipelines;
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_GUIPipelines;
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_TextPipelines;
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_ShadowPipelines;

	GFX::GfxCommandList *m_ActiveCmd = nullptr;
	RHI::TextureFormat m_ActiveColorFormat = RHI::TextureFormat::None;
	RHI::TextureFormat m_ActiveDepthFormat = RHI::TextureFormat::None;
	RHI::SampleCount m_ActiveSampleCount = RHI::SampleCount::x1;
	mat4 m_ViewProjection = mat4(1.F);
	// Per-frame command recording state — avoids redundant GPU state changes.
	GFX::GfxPipeline *m_LastBoundPipeline = nullptr;
	GFX::GfxDescriptorSet *m_LastBoundDescSet0 = nullptr;
	GFX::GfxBuffer *m_LastBoundVertexBuffer = nullptr;
	bool m_PushConstantsDirty = true;
	// Cached descriptor set pointers — updated when the batch texture changes, not per-flush.
	GFX::GfxDescriptorSet *m_CachedTextDataSet = nullptr;
	GFX::GfxDescriptorSet *m_CachedTextureSet = nullptr;

	uint32 m_QuadCount = 0;
	uint32 m_VertexOffset = 0;
	uint32 m_TextVertexOffset = 0;
	BatchType m_BatchType = BatchType::Flat;
	GFX::GfxTexture *m_BatchTexture = nullptr;
	GFX::GfxTexture *m_BatchCurveTexture = nullptr;
	GFX::GfxTexture *m_BatchBandTexture = nullptr;

	// Replay state — built during a captured Submit, consumed by ExecuteReplay.
	std::vector<ReplayEntry> m_ReplayList;
	bool m_Capturing = false;
	uint32 m_CurrentSlot = 0; // ring slot in use this frame (set by Begin)
	uint32 m_LastDirtySlot = 0; // ring slot written during last BeginCapture+Submit
	uint64 m_ReplayQuadBytes = 0;
	uint64 m_ReplayTextBytes = 0;

	Stats m_Stats;
};

} // namespace Aquila::Graphics
