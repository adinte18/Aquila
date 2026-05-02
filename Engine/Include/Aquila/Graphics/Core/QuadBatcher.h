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
	vec2 size;
	vec4 radius;
	float borderWidth;
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

	void DrawRect(const RectSpec &spec);
	void DrawSprite(const SpriteSpec &spec);

	void ResetStats() { m_Stats = {}; }
	[[nodiscard]] uint32 GetDrawCallCount() const { return m_Stats.drawCalls; }
	[[nodiscard]] uint32 GetQuadCount() const { return m_Stats.quadCount; }

	GFX::GfxPipeline &GetOrCreateFlatPipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
											  RHI::TextureFormat depthFormat);
	GFX::GfxPipeline &GetOrCreateTexturePipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
												 RHI::TextureFormat depthFormat);
	GFX::GfxPipeline &GetOrCreateGUIPipeline(RHI::TextureFormat colorFormat, RHI::SampleCount samples,
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

	enum class BatchType { Flat, GUI, Texture };

	void StartBatch();
	[[nodiscard]] mat4 BuildQuadTransform(vec2 position, vec2 size, float rotation, float depth) const;

	GFX::GfxContext &m_Ctx;

	Ref<GFX::GfxBuffer> m_VertexBuffer;
	Ref<GFX::GfxBuffer> m_IndexBuffer;
	Ref<GFX::GfxDescriptorSetLayout> m_TextureLayout;
	Ref<GFX::GfxDescriptorSet> m_TextureSet;

	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_FlatPipelines;
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_TexturePipelines;
	std::unordered_map<PipelineKey, Ref<GFX::GfxPipeline>, PipelineKeyHash> m_GUIPipelines;

	GFX::GfxCommandList *m_ActiveCmd = nullptr;
	RHI::TextureFormat m_ActiveColorFormat = RHI::TextureFormat::None;
	RHI::TextureFormat m_ActiveDepthFormat = RHI::TextureFormat::None;
	RHI::SampleCount m_ActiveSampleCount = RHI::SampleCount::x1;
	mat4 m_ViewProjection = mat4(1.F);

	std::vector<QuadVertex> m_VertexData;
	uint32 m_QuadCount = 0;
	BatchType m_BatchType = BatchType::Flat;
	GFX::GfxTexture *m_BatchTexture = nullptr;

	Stats m_Stats;
};

} // namespace Aquila::Graphics
