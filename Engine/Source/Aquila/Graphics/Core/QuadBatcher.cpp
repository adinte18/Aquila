#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/Foundation/SharedConstants.h"
#include <cstring>
#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"

using namespace Aquila;
using namespace Aquila::Graphics;

static constexpr const char *kFlatShader = AQUILA_SHADERS_DIR "2D/Flat2D.slang";
static constexpr const char *kTextureShader = AQUILA_SHADERS_DIR "2D/Sprite2D.slang";
static constexpr const char *kGUIShader = AQUILA_SHADERS_DIR "2D/GUI2D.slang";
static constexpr const char *kTextShader = AQUILA_SHADERS_DIR "2D/Text2D.slang";
static constexpr const char *kShadowShader = AQUILA_SHADERS_DIR "2D/Shadow2D.slang";

static Ref<GFX::GfxPipeline> BuildPipeline(GFX::GfxContext &ctx, const char *shaderPath,
										   const std::vector<GFX::GfxDescriptorSetLayout *> &setLayouts,
										   uint32 pushConstantSize, RHI::TextureFormat colorFormat,
										   RHI::SampleCount sampleCount, RHI::TextureFormat depthFormat,
										   RHI::VertexBindingDesc vertexLayout, bool minSampleShading = false) {
	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(shaderPath, stages, err)) {
		AQUILA_LOG_ERROR("QuadBatcher shader failed [{}]: {}", shaderPath, err);
	}

	RHI::GraphicsPipelineDesc desc{};
	for (auto &s : stages) {
		RHI::ShaderStageDesc sd{ .spirv = s.spirv, .entryPoint = s.entryPointName };
		if (s.stage == VK_SHADER_STAGE_VERTEX_BIT) {
			sd.stage = RHI::ShaderStageFlags::Vertex;
			desc.vertexShader = sd;
		} else {
			sd.stage = RHI::ShaderStageFlags::Fragment;
			desc.fragmentShader = sd;
		}
	}

	std::vector<RHI::IRHIDescriptorSetLayout *> temp;
	temp.reserve(setLayouts.size());
	for (const auto &layout : setLayouts) {
		temp.push_back(&layout->GetRHI());
	}

	desc.setLayouts = temp;
	desc.colorFormats = { colorFormat };
	desc.depthFormat = depthFormat;
	desc.topology = RHI::PrimitiveTopology::TriangleList;
	desc.raster.cullMode = RHI::CullMode::None;
	desc.raster.frontFace = RHI::FrontFace::Clockwise;
	desc.depthStencil.depthTest = false;
	desc.depthStencil.depthWrite = false;
	desc.blendAttachments = { { true } };
	desc.pushConstants = { { RHI::ShaderStageFlags::Vertex, 0, pushConstantSize } };
	desc.sampleCount = sampleCount;
	desc.minSampleShading = minSampleShading;
	desc.customVertexLayout = std::move(vertexLayout);

	return ctx.CreateGraphicsPipeline(desc);
}

static RHI::VertexBindingDesc QuadVertexLayout() {
	return RHI::VertexBindingDesc{
		.stride = sizeof(QuadVertex),
		.attributes = {
			{ 0, 0, RHI::TextureFormat::RGB32F,  offsetof(QuadVertex, position)    },
			{ 1, 0, RHI::TextureFormat::RGBA32F, offsetof(QuadVertex, color)       },
			{ 2, 0, RHI::TextureFormat::RG32F,   offsetof(QuadVertex, uv)          },
			{ 3, 0, RHI::TextureFormat::RG32F,   offsetof(QuadVertex, size)        },
			{ 4, 0, RHI::TextureFormat::RGBA32F, offsetof(QuadVertex, radius)      },
			{ 5, 0, RHI::TextureFormat::R32F,    offsetof(QuadVertex, borderWidth) },
			{ 6, 0, RHI::TextureFormat::RGBA32F, offsetof(QuadVertex, borderColor) },
			{ 7, 0, RHI::TextureFormat::R32UI,   offsetof(QuadVertex, glyphID)     },
		},
	};
}

static RHI::VertexBindingDesc TextVertexLayout() {
	return RHI::VertexBindingDesc{
		.stride = sizeof(TextVertex),
		.attributes = {
			{ 0, 0, RHI::TextureFormat::RGB32F,  offsetof(TextVertex, position) },
			{ 1, 0, RHI::TextureFormat::RGBA32F, offsetof(TextVertex, color)    },
			{ 2, 0, RHI::TextureFormat::RG32F,   offsetof(TextVertex, texcoord) },
			{ 3, 0, RHI::TextureFormat::R32F,    offsetof(TextVertex, texLoc)   },
			{ 4, 0, RHI::TextureFormat::R32F,    offsetof(TextVertex, bandMax)  },
			{ 5, 0, RHI::TextureFormat::RGBA32F, offsetof(TextVertex, banding)  },
		},
	};
}

static std::vector<uint32> GenerateQuadIndices(uint32 maxQuads) {
	std::vector<uint32> indices;
	indices.reserve(maxQuads * SharedConstants::INDICES_PER_QUAD);
	for (uint32 i = 0; i < maxQuads; ++i) {
		const uint32 base = i * SharedConstants::VERTS_PER_QUAD;
		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
		indices.push_back(base + 0);
	}
	return indices;
}

QuadBatcher::QuadBatcher(GFX::GfxContext &ctx) : m_Ctx(ctx) {
	const uint64 quadVbSize = sizeof(QuadVertex) * SharedConstants::MAX_QUADS * SharedConstants::VERTS_PER_QUAD;
	const uint64 textVbSize = sizeof(TextVertex) * SharedConstants::MAX_QUADS * SharedConstants::VERTS_PER_QUAD;

	for (uint32 i = 0; i < kRingSize; ++i) {
		m_VertexBuffers[i] = ctx.CreateBuffer({
			.size = quadVbSize,
			.usage = RHI::BufferUsage::VertexBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "QuadBatcher_VB",
		});
		m_TextVertexBuffers[i] = ctx.CreateBuffer({
			.size = textVbSize,
			.usage = RHI::BufferUsage::VertexBuffer,
			.domain = RHI::MemoryDomain::CPU_TO_GPU,
			.debugName = "QuadBatcher_TextVB",
		});
		m_MappedQuadBases[i] = static_cast<QuadVertex *>(m_VertexBuffers[i]->Map());
		m_MappedTextBases[i] = static_cast<TextVertex *>(m_TextVertexBuffers[i]->Map());
	}

	auto indices = GenerateQuadIndices(SharedConstants::MAX_QUADS);
	m_IndexBuffer = ctx.CreateBuffer({
		.size = static_cast<uint64>(sizeof(uint32) * indices.size()),
		.usage = RHI::BufferUsage::IndexBuffer | RHI::BufferUsage::TransferDst,
		.domain = RHI::MemoryDomain::CPU_TO_GPU,
		.debugName = "QuadBatcher_IB",
	});
	m_IndexBuffer->Write(indices.data(), sizeof(uint32) * indices.size());

	m_TextureLayout = ctx.CreateDescriptorSetLayout({
		.bindings = { {
			.binding = 0,
			.type = RHI::DescriptorType::CombinedImageSampler,
			.stages = RHI::ShaderStageFlags::Fragment,
			.count = 1,
		} },
	});

	m_TextDataLayout = ctx.CreateDescriptorSetLayout({
		.bindings = {
			{
				.binding = 0,
				.type    = RHI::DescriptorType::CombinedImageSampler,
				.stages  = RHI::ShaderStageFlags::Fragment,
				.count   = 1,
			},
			{
				.binding = 1,
				.type    = RHI::DescriptorType::CombinedImageSampler,
				.stages  = RHI::ShaderStageFlags::Fragment,
				.count   = 1,
			},
		},
	});

	// Prewarm all pipeline variants used by UIOverlay (BGRA8) and GameUI (RGBA16F).
	// Lazy compilation inside an execute lambda causes a multi-frame stall on first use.
	for (auto fmt : { RHI::TextureFormat::BGRA8, RHI::TextureFormat::RGBA16F }) {
		GetOrCreateFlatPipeline(fmt, RHI::SampleCount::x1, RHI::TextureFormat::None);
		GetOrCreateGUIPipeline(fmt, RHI::SampleCount::x1, RHI::TextureFormat::None);
		GetOrCreateTexturePipeline(fmt, RHI::SampleCount::x1, RHI::TextureFormat::None);
		GetOrCreateTextPipeline(fmt, RHI::SampleCount::x1, RHI::TextureFormat::None);
		GetOrCreateShadowPipeline(fmt, RHI::SampleCount::x1, RHI::TextureFormat::None);
	}
	// Prewarm x4 MSAA text pipeline for the UIOverlay pass.
	GetOrCreateTextPipeline(RHI::TextureFormat::BGRA8, RHI::SampleCount::x4, RHI::TextureFormat::None);
}

void QuadBatcher::Begin(GFX::GfxCommandList &cmd, RHI::TextureFormat colorFormat, RHI::SampleCount sampleCount,
						const mat4 &viewProjection, RHI::TextureFormat depthFormat) {
	AQUILA_ASSERT(!m_ActiveCmd, "QuadBatcher::Begin called while already recording");
	m_ActiveCmd = &cmd;
	m_ActiveColorFormat = colorFormat;
	m_ActiveSampleCount = sampleCount;
	m_ActiveDepthFormat = depthFormat;
	m_ViewProjection = viewProjection;

	const uint32 fi = m_FrameCounter % kRingSize;
	++m_FrameCounter;
	m_CurrentSlot = fi;
	m_ActiveVertexBuffer = m_VertexBuffers[fi].get();
	m_ActiveTextVertexBuffer = m_TextVertexBuffers[fi].get();
	m_QuadWritePtr = m_MappedQuadBases[fi];
	m_TextWritePtr = m_MappedTextBases[fi];

	m_VertexOffset = 0;
	m_TextVertexOffset = 0;
	m_LastBoundPipeline = nullptr;
	m_LastBoundDescSet0 = nullptr;
	m_LastBoundVertexBuffer = nullptr;
	m_PushConstantsDirty = true;

	// Index buffer never changes — bind once per frame instead of per draw call.
	cmd.BindIndexBuffer(*m_IndexBuffer);
	StartBatch();
}

void QuadBatcher::End() {
	AQUILA_ASSERT(m_ActiveCmd, "QuadBatcher::End called without Begin");
	Flush();
	if (m_Capturing) {
		m_LastDirtySlot = m_CurrentSlot;
		m_ReplayQuadBytes = static_cast<uint64>(m_VertexOffset) * sizeof(QuadVertex);
		m_ReplayTextBytes = static_cast<uint64>(m_TextVertexOffset) * sizeof(TextVertex);
		m_Capturing = false;
	}
	m_ActiveCmd = nullptr;
}

void QuadBatcher::BeginCapture() {
	m_ReplayList.clear();
	m_Capturing = true;
}

void QuadBatcher::ExecuteReplay(GFX::GfxCommandList &cmd) {
	if (m_ReplayList.empty()) {
		return;
	}

	GFX::GfxBuffer *replayQuadVB = m_VertexBuffers[m_LastDirtySlot].get();
	GFX::GfxBuffer *replayTextVB = m_TextVertexBuffers[m_LastDirtySlot].get();

	cmd.BindIndexBuffer(*m_IndexBuffer);

	GFX::GfxPipeline *lastPipeline = nullptr;
	GFX::GfxDescriptorSet *lastDescSet = nullptr;
	GFX::GfxBuffer *lastVB = nullptr;

	for (const ReplayEntry &entry : m_ReplayList) {
		if (entry.pipeline != lastPipeline) {
			cmd.BindPipeline(*entry.pipeline);
			lastPipeline = entry.pipeline;
			QuadPushConstants pc{ .viewProjection = m_ViewProjection };
			cmd.PushConstants(pc, RHI::ShaderStageFlags::Vertex);
		}

		GFX::GfxBuffer *vb = entry.isTextBuffer ? replayTextVB : replayQuadVB;
		if (vb != lastVB) {
			cmd.BindVertexBuffer(*vb, 0, 0);
			lastVB = vb;
		}

		if (entry.descSet != nullptr && entry.descSet != lastDescSet) {
			cmd.BindDescriptorSet(0, *entry.descSet);
			lastDescSet = entry.descSet;
		}

		cmd.DrawIndexed(entry.indexCount, 1, 0, entry.vertexOffset);
	}
}

GFX::GfxPipeline &QuadBatcher::GetOrCreateFlatPipeline(RHI::TextureFormat format, RHI::SampleCount samples,
													   RHI::TextureFormat depthFormat) {
	PipelineKey key{ format, samples, depthFormat };
	auto it = m_FlatPipelines.find(key);
	if (it != m_FlatPipelines.end()) {
		return *it->second;
	}
	m_FlatPipelines[key] = BuildPipeline(m_Ctx, kFlatShader, {}, sizeof(QuadPushConstants), format, samples,
										 depthFormat, QuadVertexLayout());
	return *m_FlatPipelines[key];
}

GFX::GfxPipeline &QuadBatcher::GetOrCreateTexturePipeline(RHI::TextureFormat format, RHI::SampleCount samples,
														  RHI::TextureFormat depthFormat) {
	PipelineKey key{ format, samples, depthFormat };
	auto it = m_TexturePipelines.find(key);
	if (it != m_TexturePipelines.end()) {
		return *it->second;
	}
	m_TexturePipelines[key] = BuildPipeline(m_Ctx, kTextureShader, { m_TextureLayout.get() }, sizeof(QuadPushConstants),
											format, samples, depthFormat, QuadVertexLayout());
	return *m_TexturePipelines[key];
}

GFX::GfxPipeline &QuadBatcher::GetOrCreateGUIPipeline(RHI::TextureFormat format, RHI::SampleCount samples,
													  RHI::TextureFormat depthFormat) {
	PipelineKey key{ format, samples, depthFormat };
	auto it = m_GUIPipelines.find(key);
	if (it != m_GUIPipelines.end()) {
		return *it->second;
	}
	m_GUIPipelines[key] = BuildPipeline(m_Ctx, kGUIShader, {}, sizeof(QuadPushConstants), format, samples, depthFormat,
										QuadVertexLayout());
	return *m_GUIPipelines[key];
}

GFX::GfxPipeline &QuadBatcher::GetOrCreateTextPipeline(RHI::TextureFormat format, RHI::SampleCount samples,
													   RHI::TextureFormat depthFormat) {
	PipelineKey key{ format, samples, depthFormat };
	auto it = m_TextPipelines.find(key);
	if (it != m_TextPipelines.end()) {
		return *it->second;
	}
	const bool perSampleShading = (samples != RHI::SampleCount::x1);
	m_TextPipelines[key] = BuildPipeline(m_Ctx, kTextShader, { m_TextDataLayout.get() }, sizeof(QuadPushConstants),
										 format, samples, depthFormat, TextVertexLayout(), perSampleShading);
	return *m_TextPipelines[key];
}

GFX::GfxPipeline &QuadBatcher::GetOrCreateShadowPipeline(RHI::TextureFormat format, RHI::SampleCount samples,
														 RHI::TextureFormat depthFormat) {
	PipelineKey key{ format, samples, depthFormat };
	auto it = m_ShadowPipelines.find(key);
	if (it != m_ShadowPipelines.end()) {
		return *it->second;
	}
	m_ShadowPipelines[key] = BuildPipeline(m_Ctx, kShadowShader, {}, sizeof(QuadPushConstants), format, samples,
										   depthFormat, QuadVertexLayout());
	return *m_ShadowPipelines[key];
}

void QuadBatcher::DrawShadow(const ShadowSpec &spec) {
	if (m_BatchTexture != nullptr || m_BatchType != BatchType::Shadow) {
		Flush();
		StartBatch();
	}
	if (m_QuadCount >= SharedConstants::MAX_QUADS) {
		Flush();
		StartBatch();
	}
	m_BatchType = BatchType::Shadow;

	static constexpr vec2 kUVs[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

	const float x0 = spec.position.x, y0 = spec.position.y;
	const float x1 = x0 + spec.size.x, y1 = y0 + spec.size.y;
	const float z = spec.depth;
	const vec4 enc = { spec.offset.x, spec.offset.y, spec.originalHalfSize.x, spec.originalHalfSize.y };

	QuadVertex *v = m_QuadWritePtr;
	m_QuadWritePtr += 4;

	v[0] = { .position = { x0, y0, z },
			 .color = spec.color,
			 .uv = kUVs[0],
			 .size = spec.size,
			 .radius = spec.radius,
			 .borderWidth = spec.blur,
			 .borderColor = enc };
	v[1] = { .position = { x1, y0, z },
			 .color = spec.color,
			 .uv = kUVs[1],
			 .size = spec.size,
			 .radius = spec.radius,
			 .borderWidth = spec.blur,
			 .borderColor = enc };
	v[2] = { .position = { x1, y1, z },
			 .color = spec.color,
			 .uv = kUVs[2],
			 .size = spec.size,
			 .radius = spec.radius,
			 .borderWidth = spec.blur,
			 .borderColor = enc };
	v[3] = { .position = { x0, y1, z },
			 .color = spec.color,
			 .uv = kUVs[3],
			 .size = spec.size,
			 .radius = spec.radius,
			 .borderWidth = spec.blur,
			 .borderColor = enc };

	++m_QuadCount;
	++m_Stats.quadCount;
}

void QuadBatcher::DrawRect(const RectSpec &spec) {
	const bool hasRadius = glm::any(glm::greaterThan(spec.radius, vec4(0.F)));
	const BatchType needed = (hasRadius || spec.borderWidth > 0.F) ? BatchType::GUI : BatchType::Flat;

	if (m_BatchTexture != nullptr || m_BatchType != needed) {
		Flush();
		StartBatch();
	}
	if (m_QuadCount >= SharedConstants::MAX_QUADS) {
		Flush();
		StartBatch();
	}
	m_BatchType = needed;

	static constexpr vec2 kUVs[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

	QuadVertex *v = m_QuadWritePtr;
	m_QuadWritePtr += 4;

	if (spec.rotation != 0.F) {
		static constexpr vec4 kLocalCorners[4] = {
			{ -0.5F, -0.5F, 0.F, 1.F },
			{ 0.5F, -0.5F, 0.F, 1.F },
			{ 0.5F, 0.5F, 0.F, 1.F },
			{ -0.5F, 0.5F, 0.F, 1.F },
		};
		mat4 transform = BuildQuadTransform(spec.position, spec.size, spec.rotation, spec.depth);
		for (uint32 i = 0; i < SharedConstants::VERTS_PER_QUAD; ++i) {
			vec4 worldPos = transform * kLocalCorners[i];
			v[i] = { .position = vec3(worldPos),
					 .color = spec.color,
					 .uv = kUVs[i],
					 .size = spec.size,
					 .radius = spec.radius,
					 .borderWidth = spec.borderWidth,
					 .borderColor = spec.borderColor };
		}
	} else {
		const float x0 = spec.position.x, y0 = spec.position.y;
		const float x1 = x0 + spec.size.x, y1 = y0 + spec.size.y;
		const float z = spec.depth;
		v[0] = { .position = { x0, y0, z },
				 .color = spec.color,
				 .uv = kUVs[0],
				 .size = spec.size,
				 .radius = spec.radius,
				 .borderWidth = spec.borderWidth,
				 .borderColor = spec.borderColor };
		v[1] = { .position = { x1, y0, z },
				 .color = spec.color,
				 .uv = kUVs[1],
				 .size = spec.size,
				 .radius = spec.radius,
				 .borderWidth = spec.borderWidth,
				 .borderColor = spec.borderColor };
		v[2] = { .position = { x1, y1, z },
				 .color = spec.color,
				 .uv = kUVs[2],
				 .size = spec.size,
				 .radius = spec.radius,
				 .borderWidth = spec.borderWidth,
				 .borderColor = spec.borderColor };
		v[3] = { .position = { x0, y1, z },
				 .color = spec.color,
				 .uv = kUVs[3],
				 .size = spec.size,
				 .radius = spec.radius,
				 .borderWidth = spec.borderWidth,
				 .borderColor = spec.borderColor };
	}

	++m_QuadCount;
	++m_Stats.quadCount;
}

void QuadBatcher::DrawSprite(const SpriteSpec &spec) {
	if (spec.texture != nullptr && !spec.texture->IsReady()) {
		return;
	}
	if (m_BatchTexture != spec.texture) {
		Flush();
		StartBatch();
		m_BatchTexture = spec.texture;
		if (spec.texture != nullptr) {
			m_CachedTextureSet = &GetOrCreateTextureSet(*spec.texture);
		}
	}
	if (m_QuadCount >= SharedConstants::MAX_QUADS) {
		Flush();
		StartBatch();
	}

	const vec2 uvs[4] = {
		{ spec.uvMin.x, spec.uvMin.y },
		{ spec.uvMax.x, spec.uvMin.y },
		{ spec.uvMax.x, spec.uvMax.y },
		{ spec.uvMin.x, spec.uvMax.y },
	};

	QuadVertex *v = m_QuadWritePtr;
	m_QuadWritePtr += 4;

	if (spec.rotation != 0.F) {
		static constexpr vec4 kLocalCorners[4] = {
			{ -0.5F, -0.5F, 0.F, 1.F },
			{ 0.5F, -0.5F, 0.F, 1.F },
			{ 0.5F, 0.5F, 0.F, 1.F },
			{ -0.5F, 0.5F, 0.F, 1.F },
		};
		mat4 transform = BuildQuadTransform(spec.position, spec.size, spec.rotation, spec.depth);
		for (uint32 i = 0; i < SharedConstants::VERTS_PER_QUAD; ++i) {
			vec4 worldPos = transform * kLocalCorners[i];
			v[i] = { .position = vec3(worldPos), .color = spec.tint, .uv = uvs[i] };
		}
	} else {
		const float x0 = spec.position.x, y0 = spec.position.y;
		const float x1 = x0 + spec.size.x, y1 = y0 + spec.size.y;
		const float z = spec.depth;
		v[0] = { .position = { x0, y0, z }, .color = spec.tint, .uv = uvs[0] };
		v[1] = { .position = { x1, y0, z }, .color = spec.tint, .uv = uvs[1] };
		v[2] = { .position = { x1, y1, z }, .color = spec.tint, .uv = uvs[2] };
		v[3] = { .position = { x0, y1, z }, .color = spec.tint, .uv = uvs[3] };
	}

	++m_QuadCount;
	++m_Stats.quadCount;
}

void QuadBatcher::DrawGlyph(const GlyphSpec &spec) {
	if (m_BatchType != BatchType::Text || m_BatchCurveTexture != spec.curveTexture) {
		Flush();
		StartBatch();
		m_BatchType = BatchType::Text;
		m_BatchCurveTexture = spec.curveTexture;
		m_BatchBandTexture = spec.bandTexture;
		m_CachedTextDataSet = &GetOrCreateTextDataSet(*spec.curveTexture, *spec.bandTexture);
	}
	if (m_QuadCount >= SharedConstants::MAX_QUADS) {
		Flush();
		StartBatch();
		m_BatchType = BatchType::Text;
		m_BatchCurveTexture = spec.curveTexture;
		m_BatchBandTexture = spec.bandTexture;
		m_CachedTextDataSet = &GetOrCreateTextDataSet(*spec.curveTexture, *spec.bandTexture);
	}

	const float texLoc = std::bit_cast<float>((spec.glyphLocX & 0xFFFFu) | ((spec.glyphLocY & 0xFFFFu) << 16u));
	const float bandMax = std::bit_cast<float>((spec.bandMaxX & 0xFFu) | ((spec.bandMaxY & 0xFFu) << 16u));

	const float emY0 = spec.flipY ? spec.emMin.y : spec.emMax.y;
	const float emY1 = spec.flipY ? spec.emMax.y : spec.emMin.y;

	const float x0 = spec.position.x, y0 = spec.position.y;
	const float x1 = x0 + spec.size.x, y1 = y0 + spec.size.y;
	const float z = spec.depth;

	TextVertex *v = m_TextWritePtr;
	m_TextWritePtr += 4;

	v[0] = { .position = { x0, y0, z },
			 .color = spec.color,
			 .texcoord = { spec.emMin.x, emY0 },
			 .texLoc = texLoc,
			 .bandMax = bandMax,
			 .banding = spec.banding };
	v[1] = { .position = { x1, y0, z },
			 .color = spec.color,
			 .texcoord = { spec.emMax.x, emY0 },
			 .texLoc = texLoc,
			 .bandMax = bandMax,
			 .banding = spec.banding };
	v[2] = { .position = { x1, y1, z },
			 .color = spec.color,
			 .texcoord = { spec.emMax.x, emY1 },
			 .texLoc = texLoc,
			 .bandMax = bandMax,
			 .banding = spec.banding };
	v[3] = { .position = { x0, y1, z },
			 .color = spec.color,
			 .texcoord = { spec.emMin.x, emY1 },
			 .texLoc = texLoc,
			 .bandMax = bandMax,
			 .banding = spec.banding };

	++m_QuadCount;
	++m_Stats.quadCount;
}

void QuadBatcher::Flush() {
	if (m_QuadCount == 0) {
		return;
	}

	auto bindPipelineIfChanged = [&](GFX::GfxPipeline &pipeline) {
		if (&pipeline != m_LastBoundPipeline) {
			m_ActiveCmd->BindPipeline(pipeline);
			m_LastBoundPipeline = &pipeline;
			// Each pipeline has its own VkPipelineLayout; push constants are not
			// preserved across incompatible layouts, so re-push on every switch.
			m_PushConstantsDirty = true;
		}
	};

	auto bindDescSet0IfChanged = [&](GFX::GfxDescriptorSet &set) {
		if (&set != m_LastBoundDescSet0) {
			m_ActiveCmd->BindDescriptorSet(0, set);
			m_LastBoundDescSet0 = &set;
		}
	};

	auto bindVertexBufferIfChanged = [&](GFX::GfxBuffer &buf) {
		if (&buf != m_LastBoundVertexBuffer) {
			m_ActiveCmd->BindVertexBuffer(buf, 0, 0);
			m_LastBoundVertexBuffer = &buf;
		}
	};

	if (m_BatchType == BatchType::Text) {
		bindPipelineIfChanged(GetOrCreateTextPipeline(m_ActiveColorFormat, m_ActiveSampleCount, m_ActiveDepthFormat));

		if (m_PushConstantsDirty) {
			QuadPushConstants pc{ .viewProjection = m_ViewProjection };
			m_ActiveCmd->PushConstants(pc, RHI::ShaderStageFlags::Vertex);
			m_PushConstantsDirty = false;
		}

		bindVertexBufferIfChanged(*m_ActiveTextVertexBuffer);
		bindDescSet0IfChanged(*m_CachedTextDataSet);

		const uint32 indexCount = m_QuadCount * SharedConstants::INDICES_PER_QUAD;
		const int32 vertexOff = static_cast<int32>(m_TextVertexOffset);
		m_ActiveCmd->DrawIndexed(indexCount, 1, 0, vertexOff);
		++m_Stats.drawCalls;

		if (m_Capturing) {
			m_ReplayList.push_back({ m_LastBoundPipeline, m_CachedTextDataSet, true, indexCount, vertexOff });
		}

		m_TextVertexOffset += m_QuadCount * SharedConstants::VERTS_PER_QUAD;
	} else {
		GFX::GfxPipeline &pipeline = [&]() -> GFX::GfxPipeline & {
			if (m_BatchTexture != nullptr) {
				return GetOrCreateTexturePipeline(m_ActiveColorFormat, m_ActiveSampleCount, m_ActiveDepthFormat);
			}
			if (m_BatchType == BatchType::GUI) {
				return GetOrCreateGUIPipeline(m_ActiveColorFormat, m_ActiveSampleCount, m_ActiveDepthFormat);
			}
			if (m_BatchType == BatchType::Shadow) {
				return GetOrCreateShadowPipeline(m_ActiveColorFormat, m_ActiveSampleCount, m_ActiveDepthFormat);
			}
			return GetOrCreateFlatPipeline(m_ActiveColorFormat, m_ActiveSampleCount, m_ActiveDepthFormat);
		}();

		bindPipelineIfChanged(pipeline);

		if (m_PushConstantsDirty) {
			QuadPushConstants pc{ .viewProjection = m_ViewProjection };
			m_ActiveCmd->PushConstants(pc, RHI::ShaderStageFlags::Vertex);
			m_PushConstantsDirty = false;
		}

		bindVertexBufferIfChanged(*m_ActiveVertexBuffer);

		GFX::GfxDescriptorSet *boundSet = nullptr;
		if (m_BatchTexture != nullptr) {
			bindDescSet0IfChanged(*m_CachedTextureSet);
			boundSet = m_CachedTextureSet;
		}

		const uint32 indexCount = m_QuadCount * SharedConstants::INDICES_PER_QUAD;
		const int32 vertexOff = static_cast<int32>(m_VertexOffset);
		m_ActiveCmd->DrawIndexed(indexCount, 1, 0, vertexOff);
		++m_Stats.drawCalls;

		if (m_Capturing) {
			m_ReplayList.push_back({ m_LastBoundPipeline, boundSet, false, indexCount, vertexOff });
		}

		m_VertexOffset += m_QuadCount * SharedConstants::VERTS_PER_QUAD;
	}
}

GFX::GfxDescriptorSet &QuadBatcher::GetOrCreateTextureSet(GFX::GfxTexture &texture) {
	auto it = m_TextureSetCache.find(&texture);
	if (it != m_TextureSetCache.end()) {
		return *it->second;
	}
	auto set = m_Ctx.AllocateDescriptorSet(*m_TextureLayout);
	set->SetTexture(0, texture);
	set->Flush();
	m_TextureSetCache[&texture] = std::move(set);
	return *m_TextureSetCache[&texture];
}

GFX::GfxDescriptorSet &QuadBatcher::GetOrCreateTextDataSet(GFX::GfxTexture &curveTexture,
														   GFX::GfxTexture &bandTexture) {
	auto it = m_TextDataSetCache.find(&curveTexture);
	if (it != m_TextDataSetCache.end()) {
		return *it->second;
	}
	auto set = m_Ctx.AllocateDescriptorSet(*m_TextDataLayout);
	set->SetTexture(0, curveTexture);
	set->SetTexture(1, bandTexture);
	set->Flush();
	m_TextDataSetCache[&curveTexture] = std::move(set);
	return *m_TextDataSetCache[&curveTexture];
}

void QuadBatcher::StartBatch() {
	m_QuadCount = 0;
	m_BatchType = BatchType::Flat;
	m_BatchTexture = nullptr;
	m_BatchCurveTexture = nullptr;
	m_BatchBandTexture = nullptr;
}

mat4 QuadBatcher::BuildQuadTransform(vec2 position, vec2 size, float rotation, float depth) const {
	vec2 center = position + size * 0.5F;
	mat4 t = glm::translate(mat4(1.F), vec3(center, depth));
	if (rotation != 0.F) {
		t = glm::rotate(t, rotation, vec3(0.F, 0.F, 1.F));
	}
	t = glm::scale(t, vec3(size, 1.F));
	return t;
}
