#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"
#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::Graphics {

Ref<GFX::GfxPipeline> MaterialFactory::BuildPipeline(GFX::GfxContext &ctx, Shader::ShaderProgram &program,
													 const MaterialCreateInfo &info) {
	auto *sceneLayout = &Rendering::SceneFrameData::Get()->GetLayout().GetRHI();

	if (!program.GetStageDesc(RHI::ShaderStageFlags::Vertex).spirv.empty() == false &&
		!program.GetStageDesc(RHI::ShaderStageFlags::Compute).spirv.empty()) {
		RHI::ComputePipelineDesc desc{};
		desc.computeShader = program.GetStageDesc(RHI::ShaderStageFlags::Compute);
		desc.setLayouts = { sceneLayout };
		if (program.m_DescriptorSetLayout) {
			desc.setLayouts.push_back(&program.m_DescriptorSetLayout->GetRHI());
		}
		desc.pushConstants = { { RHI::ShaderStageFlags::Compute, 0, info.pushConstantSize } };
		return ctx.CreateComputePipeline(desc);
	}

	RHI::GraphicsPipelineDesc desc{};
	desc.vertexShader = program.GetStageDesc(RHI::ShaderStageFlags::Vertex);
	desc.fragmentShader = program.GetStageDesc(RHI::ShaderStageFlags::Fragment);
	desc.topology = RHI::PrimitiveTopology::TriangleList;
	desc.colorFormats = info.colorFormats;
	desc.depthFormat = info.depthFormat;

	desc.raster.cullMode = info.cullMode;
	desc.raster.frontFace = info.frontFace;

	desc.depthStencil.depthTest = info.depthTest;
	desc.depthStencil.depthWrite = info.depthWrite;

	if (info.blendEnabled) {
		RHI::BlendAttachmentDesc blend{};
		blend.enable = true;
		blend.srcColor = RHI::BlendFactor::SrcAlpha;
		blend.dstColor = RHI::BlendFactor::OneMinusSrcAlpha;
		blend.colorOp = RHI::BlendOp::Add;
		blend.srcAlpha = RHI::BlendFactor::One;
		blend.dstAlpha = RHI::BlendFactor::Zero;
		blend.alphaOp = RHI::BlendOp::Add;
		desc.blendAttachments.assign(info.colorFormats.size(), blend);
	}

	desc.setLayouts = { sceneLayout };
	if (program.m_DescriptorSetLayout) {
		desc.setLayouts.push_back(&program.m_DescriptorSetLayout->GetRHI());
	}

	desc.pushConstants = { { RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment, 0,
							 info.pushConstantSize } };

	desc.debugName = program.m_Name;
	return ctx.CreateGraphicsPipeline(desc);
}

Ref<Material> MaterialFactory::Create(GFX::GfxContext &ctx, const std::string &shaderPath, MaterialCreateInfo info) {
	auto &entry = m_Entries[shaderPath];

	if (!entry.program) {
		entry.program = Ref<Shader::ShaderProgram>(new Shader::ShaderProgram(ctx, shaderPath));
		entry.info = info;

		std::string err;
		if (!entry.program->AddStageFromSlang(shaderPath, err)) {
			AQUILA_LOG_ERROR("MaterialFactory: compile failed for '{}': {}", shaderPath, err);
			m_Entries.erase(shaderPath);
			return nullptr;
		}
		if (!entry.program->Reflect()) {
			AQUILA_LOG_ERROR("MaterialFactory: reflection failed for '{}'", shaderPath);
			m_Entries.erase(shaderPath);
			return nullptr;
		}

		m_Watcher.WatchSlangFile(shaderPath, shaderPath);
	}

	auto pipeline = BuildPipeline(ctx, *entry.program, info);
	if (!pipeline) {
		AQUILA_LOG_ERROR("MaterialFactory: pipeline creation failed for '{}'", shaderPath);
		return nullptr;
	}

	Ref<Material> mat = Material::CreateFromShader(ctx, *entry.program, pipeline);
	mat->SetType(info.type);
	mat->m_ShaderPath = shaderPath;

	entry.instances.push_back(mat);
	return mat;
}

void MaterialFactory::RebuildEntry(GFX::GfxContext &ctx, Entry &entry, const std::string &shaderPath) {
	std::string err;
	Ref<GFX::GfxDescriptorSetLayout> newLayout;

	if (!entry.program->Reload(err, newLayout)) {
		AQUILA_LOG_ERROR("MaterialFactory: hot-reload compile failed for '{}': {}", shaderPath, err);
		return;
	}

	auto newPipeline = BuildPipeline(ctx, *entry.program, entry.info);
	if (!newPipeline) {
		AQUILA_LOG_ERROR("MaterialFactory: hot-reload pipeline failed for '{}'", shaderPath);
		return;
	}

	entry.program->CommitNewLayout(newLayout);

	ctx.WaitIdle();

	auto it = entry.instances.begin();
	while (it != entry.instances.end()) {
		if (auto mat = it->lock()) {
			mat->ReplacePipeline(newPipeline, newLayout);
			++it;
		} else {
			it = entry.instances.erase(it);
		}
	}

	AQUILA_LOG_INFO("MaterialFactory: hot-reloaded '{}' ({} instances)", shaderPath, entry.instances.size());
}

void MaterialFactory::Tick(GFX::GfxContext &ctx) {
	auto changed = m_Watcher.CheckForChanges();
	for (const auto &path : changed) {
		auto it = m_Entries.find(path);
		if (it != m_Entries.end()) {
			RebuildEntry(ctx, it->second, path);
		}
	}
}

} // namespace Aquila::Graphics
