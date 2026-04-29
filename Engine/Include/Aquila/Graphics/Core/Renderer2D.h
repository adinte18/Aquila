#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::Graphics {

struct QuadVertex {
	vec3 position;
	vec4 color;
	vec2 uv;
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

// Renderer2D
//
// Stateless-per-pass 2D quad batcher.  Call Begin() once per render pass to
// establish context, emit draw specs, then End() to flush any remaining batch.
//
// The renderer holds no assumptions about frame lifecycle or pass ordering —
// it is fully compatible with RenderGraph execute callbacks:
//
//   graph.AddPass("UI",
//     [&](RGPassBuilder& b) { hColor = b.SetColorAttachment(0, hColor, ...); },
//     [&](GfxCommandList& cmd, RGRegistry& reg) {
//         renderer2D.Begin(cmd, RHI::TextureFormat::RGBA16F, RHI::SampleCount::x1, orthoVP);
//         renderer2D.DrawRect({ .position={100,100}, .size={200,50}, .color=kRed });
//         renderer2D.End();
//     });

class Renderer2D {
  public:
	explicit Renderer2D(GFX::GfxContext &ctx);
	~Renderer2D() = default;
	AQUILA_NONCOPYABLE(Renderer2D);
	AQUILA_NONMOVEABLE(Renderer2D);

	/// Begin a new 2D drawing session for the current pass.
	/// Must be called inside a renderpass (between GfxRenderPass::Begin and End).
	/// depthFormat must match the depth attachment format active in the render pass
	/// (even when depth testing is disabled) to satisfy Vulkan pipeline compatibility.
	void Begin(GFX::GfxCommandList &cmd, RHI::TextureFormat colorFormat, RHI::SampleCount sampleCount,
			   const mat4 &viewProjection, RHI::TextureFormat depthFormat = RHI::TextureFormat::None);

	/// Flush any remaining batch and release the active command list reference.
	void End();

	void DrawRect(const RectSpec &spec);
	void DrawSprite(const SpriteSpec &spec);

	void ResetStats() { m_Stats = {}; }
	[[nodiscard]] uint32 GetDrawCallCount() const { return m_Stats.drawCalls; }
	[[nodiscard]] uint32 GetQuadCount() const { return m_Stats.quadCount; }

	GFX::GfxPipeline &GetOrCreateFlatPipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
											  RHI::TextureFormat depthFormat);
	GFX::GfxPipeline &GetOrCreateTexturePipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
												 RHI::TextureFormat depthFormat);

  private:
	struct Stats {
		uint32 drawCalls = 0;
		uint32 quadCount = 0;
	};

	struct PipelineKey {
		RHI::TextureFormat colorFormat;
		RHI::SampleCount sampleCount;
		RHI::TextureFormat depthFormat; // must match active render pass depth attachment
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

	void Flush();
	void StartBatch();
	[[nodiscard]] mat4 BuildQuadTransform(vec2 position, vec2 size, float rotation, float depth) const;

	GFX::GfxContext &m_Ctx;

	// GPU resources
	Ref<GFX::GfxBuffer> m_VertexBuffer;
	Ref<GFX::GfxBuffer> m_IndexBuffer;
	Ref<GFX::GfxDescriptorSetLayout> m_TextureLayout;
	Ref<GFX::GfxDescriptorSet> m_TextureSet; // Updated per textured batch flush

	// Pipeline cache
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_FlatPipelines;
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_TexturePipelines;

	// Per-Begin state
	GFX::GfxCommandList *m_ActiveCmd = nullptr;
	RHI::TextureFormat m_ActiveColorFormat = RHI::TextureFormat::None;
	RHI::TextureFormat m_ActiveDepthFormat = RHI::TextureFormat::None;
	RHI::SampleCount m_ActiveSampleCount = RHI::SampleCount::x1;
	mat4 m_ViewProjection = mat4(1.F);

	// CPU-side batch
	std::vector<QuadVertex> m_VertexData;
	uint32 m_QuadCount = 0;
	GFX::GfxTexture *m_BatchTexture = nullptr;

	Stats m_Stats;
};

} // namespace Aquila::Graphics
