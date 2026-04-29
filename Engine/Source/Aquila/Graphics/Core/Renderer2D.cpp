#include "Aquila/Graphics/Core/Renderer2D.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"

using namespace Aquila;
using namespace Aquila::Graphics;

static constexpr const char *kFlatShader = AQUILA_SHADERS_DIR "2D/Flat2D.slang";
static constexpr const char *kTextureShader = AQUILA_SHADERS_DIR "2D/Sprite2D.slang";

static Ref<GFX::GfxPipeline> BuildPipeline(GFX::GfxContext &ctx, const char *shaderPath,
										   const std::vector<GFX::GfxDescriptorSetLayout *> &setLayouts,
										   uint32 pushConstantSize, RHI::TextureFormat colorFormat,
										   RHI::SampleCount sampleCount, RHI::TextureFormat depthFormat) {
	std::vector<RHI::VulkanCompiledStage> stages;
	std::string err;
	if (!RHI::VulkanShaderCompiler::CompileFile(shaderPath, stages, err)) {
		AQUILA_LOG_ERROR("Renderer2D shader failed [{}]: {}", shaderPath, err);
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
	for (const auto &setLayouts : setLayouts) {
		temp.push_back(&setLayouts->GetRHI());
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
	desc.customVertexLayout = RHI::VertexBindingDesc{
		.stride = sizeof(QuadVertex),
		.attributes = {
			{ 0, 0, RHI::TextureFormat::RGB32F, offsetof(QuadVertex, position) },
			{ 1, 0, RHI::TextureFormat::RGBA32F, offsetof(QuadVertex, color) },
			{ 2, 0, RHI::TextureFormat::RG32F, offsetof(QuadVertex, uv) },
		},
	};

	return ctx.CreateGraphicsPipeline(desc);
}

static std::vector<uint32> GenerateQuadIndices(uint32 maxQuads) {
	std::vector<uint32> indices;
	indices.reserve(maxQuads * SharedConstants::INDICES_PER_QUAD);
	for (uint32 i = 0; i < maxQuads; ++i) {
		uint32 base = i * SharedConstants::VERTS_PER_QUAD;
		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
		indices.push_back(base + 0);
	}
	return indices;
}

Renderer2D::Renderer2D(GFX::GfxContext &ctx) : m_Ctx(ctx) {
	m_VertexBuffer = ctx.CreateBuffer({
		.size = sizeof(QuadVertex) * SharedConstants::MAX_QUADS * SharedConstants::VERTS_PER_QUAD,
		.usage = RHI::BufferUsage::VertexBuffer,
		.domain = RHI::MemoryDomain::CPU_TO_GPU,
		.debugName = "Renderer2D_VB",
	});

	auto indices = GenerateQuadIndices(SharedConstants::MAX_QUADS);
	m_IndexBuffer = ctx.CreateBuffer({
		.size = static_cast<uint64>(sizeof(uint32) * indices.size()),
		.usage = RHI::BufferUsage::IndexBuffer | RHI::BufferUsage::TransferDst,
		.domain = RHI::MemoryDomain::CPU_TO_GPU,
		.debugName = "Renderer2D_IB",
	});
	m_IndexBuffer->Write(indices.data(), sizeof(uint32) * indices.size());

	m_VertexData.reserve(SharedConstants::MAX_QUADS * SharedConstants::VERTS_PER_QUAD);

	m_TextureLayout = ctx.CreateDescriptorSetLayout({
		.bindings = { {
			.binding = 0,
			.type = RHI::DescriptorType::CombinedImageSampler,
			.stages = RHI::ShaderStageFlags::Fragment,
			.count = 1,
		} },
	});

	m_TextureSet = ctx.AllocateDescriptorSet(*m_TextureLayout);

	// Pre-warm common formats with no depth (depth-bearing variants compiled on first Begin)
	GetOrCreateFlatPipeline(RHI::TextureFormat::BGRA8, RHI::SampleCount::x1, RHI::TextureFormat::None);
	GetOrCreateFlatPipeline(RHI::TextureFormat::RGBA16F, RHI::SampleCount::x1, RHI::TextureFormat::None);
}

void Renderer2D::Begin(GFX::GfxCommandList &cmd, RHI::TextureFormat colorFormat, RHI::SampleCount sampleCount,
					   const mat4 &viewProjection, RHI::TextureFormat depthFormat) {
	AQUILA_ASSERT(!m_ActiveCmd, "Renderer2D::Begin called while already recording");
	m_ActiveCmd = &cmd;
	m_ActiveColorFormat = colorFormat;
	m_ActiveSampleCount = sampleCount;
	m_ActiveDepthFormat = depthFormat;
	m_ViewProjection = viewProjection;
	StartBatch();
}

void Renderer2D::End() {
	AQUILA_ASSERT(m_ActiveCmd, "Renderer2D::End called without Begin");
	Flush();
	m_ActiveCmd = nullptr;
}

GFX::GfxPipeline &Renderer2D::GetOrCreateFlatPipeline(RHI::TextureFormat format, RHI::SampleCount samples,
													  RHI::TextureFormat depthFormat) {
	PipelineKey key{ .colorFormat = format, .sampleCount = samples, .depthFormat = depthFormat };
	auto it = m_FlatPipelines.find(key);
	if (it != m_FlatPipelines.end()) {
		return *it->second;
	}
	m_FlatPipelines[key] =
		BuildPipeline(m_Ctx, kFlatShader, {}, sizeof(QuadPushConstants), format, samples, depthFormat);
	return *m_FlatPipelines[key];
}

GFX::GfxPipeline &Renderer2D::GetOrCreateTexturePipeline(RHI::TextureFormat format, RHI::SampleCount samples,
														 RHI::TextureFormat depthFormat) {
	PipelineKey key{ .colorFormat = format, .sampleCount = samples, .depthFormat = depthFormat };
	auto it = m_TexturePipelines.find(key);
	if (it != m_TexturePipelines.end()) {
		return *it->second;
	}
	m_TexturePipelines[key] = BuildPipeline(m_Ctx, kTextureShader, { m_TextureLayout.get() }, sizeof(QuadPushConstants),
											format, samples, depthFormat);
	return *m_TexturePipelines[key];
}

void Renderer2D::DrawRect(const RectSpec &spec) {
	if (m_BatchTexture != nullptr) {
		Flush();
		StartBatch();
	}
	if (m_QuadCount >= SharedConstants::MAX_QUADS) {
		Flush();
		StartBatch();
	}

	vec2 center = { spec.position.x - (spec.size.x * 0.5F), spec.position.y - (spec.size.y * 0.5F) };
	mat4 transform = BuildQuadTransform(center, spec.size, spec.rotation, spec.depth);

	static constexpr vec4 kLocalCorners[4] = {
		{ -0.5F, -0.5F, 0.F, 1.F },
		{ 0.5F, -0.5F, 0.F, 1.F },
		{ 0.5F, 0.5F, 0.F, 1.F },
		{ -0.5F, 0.5F, 0.F, 1.F },
	};
	static constexpr vec2 kUVs[4] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };

	for (uint32 i = 0; i < SharedConstants::VERTS_PER_QUAD; ++i) {
		vec4 worldPos = transform * kLocalCorners[i];
		m_VertexData.push_back({ vec3(worldPos), spec.color, kUVs[i] });
	}
	++m_QuadCount;
	++m_Stats.quadCount;
}

void Renderer2D::DrawSprite(const SpriteSpec &spec) {
	if (m_BatchTexture != spec.texture) {
		Flush();
		StartBatch();
		m_BatchTexture = spec.texture;
	}
	if (m_QuadCount >= SharedConstants::MAX_QUADS) {
		Flush();
		StartBatch();
	}

	mat4 transform = BuildQuadTransform(spec.position, spec.size, spec.rotation, spec.depth);

	vec2 uvs[4] = {
		{ spec.uvMin.x, spec.uvMin.y },
		{ spec.uvMax.x, spec.uvMin.y },
		{ spec.uvMax.x, spec.uvMax.y },
		{ spec.uvMin.x, spec.uvMax.y },
	};
	static constexpr vec4 kLocalCorners[4] = {
		{ -0.5F, -0.5F, 0.F, 1.F },
		{ 0.5F, -0.5F, 0.F, 1.F },
		{ 0.5F, 0.5F, 0.F, 1.F },
		{ -0.5F, 0.5F, 0.F, 1.F },
	};

	for (uint32 i = 0; i < SharedConstants::VERTS_PER_QUAD; ++i) {
		vec4 worldPos = transform * kLocalCorners[i];
		m_VertexData.push_back({ vec3(worldPos), spec.tint, uvs[i] });
	}
	++m_QuadCount;
	++m_Stats.quadCount;
}

void Renderer2D::Flush() {
	if (m_QuadCount == 0) {
		return;
	}

	m_VertexBuffer->Write(m_VertexData.data(), sizeof(QuadVertex) * m_VertexData.size());

	GFX::GfxPipeline &pipeline =
		(m_BatchTexture != nullptr)
			? GetOrCreateTexturePipeline(m_ActiveColorFormat, m_ActiveSampleCount, m_ActiveDepthFormat)
			: GetOrCreateFlatPipeline(m_ActiveColorFormat, m_ActiveSampleCount, m_ActiveDepthFormat);

	m_ActiveCmd->BindPipeline(pipeline);

	QuadPushConstants pc{ .viewProjection = m_ViewProjection };
	m_ActiveCmd->PushConstants(pc, RHI::ShaderStageFlags::Vertex);

	m_ActiveCmd->BindVertexBuffer(*m_VertexBuffer);
	m_ActiveCmd->BindIndexBuffer(*m_IndexBuffer);

	if (m_BatchTexture != nullptr) {
		m_TextureSet->SetTexture(0, *m_BatchTexture);
		m_TextureSet->Flush();
		m_ActiveCmd->BindDescriptorSet(0, *m_TextureSet);
	}

	m_ActiveCmd->DrawIndexed(m_QuadCount * SharedConstants::INDICES_PER_QUAD);
	++m_Stats.drawCalls;
}

void Renderer2D::StartBatch() {
	m_QuadCount = 0;
	m_BatchTexture = nullptr;
	m_VertexData.clear();
}

mat4 Renderer2D::BuildQuadTransform(vec2 position, vec2 size, float rotation, float depth) const {
	vec2 center = position + size * 0.5F;
	mat4 t = glm::translate(mat4(1.F), vec3(center, depth));
	if (rotation != 0.F) {
		t = glm::rotate(t, rotation, vec3(0.F, 0.F, 1.F));
	}
	t = glm::scale(t, vec3(size, 1.F));
	return t;
}
