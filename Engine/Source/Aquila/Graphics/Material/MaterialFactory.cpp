#include "Aquila/Graphics/Material/MaterialFactory.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"
#include "Aquila/Graphics/Shader/ShaderHotReload.h"
#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::Graphics {

MaterialFactory::~MaterialFactory() {
	if (!Shader::ShaderHotReload::is_alive()) {
		return;
	}
	for (auto &[path, entry] : m_entries) {
		if (entry.watch_id != 0) {
			Shader::ShaderHotReload::get()->unregister(entry.watch_id);
		}
	}
}

Ref<GFX::GfxPipeline> MaterialFactory::build_pipeline(GFX::GfxContext &ctx, Shader::ShaderProgram &program,
													  const MaterialCreateInfo &info) {
	auto *scene_layout = &Rendering::SceneFrameData::get()->get_layout().get_rhi();

	if (!program.get_stage_desc(RHI::ShaderStageFlags::Vertex).spirv.empty() == false &&
		!program.get_stage_desc(RHI::ShaderStageFlags::Compute).spirv.empty()) {
		RHI::ComputePipelineDesc desc{};
		desc.compute_shader = program.get_stage_desc(RHI::ShaderStageFlags::Compute);
		desc.set_layouts = { scene_layout };
		if (program.m_descriptor_set_layout) {
			desc.set_layouts.push_back(&program.m_descriptor_set_layout->get_rhi());
		}
		desc.push_constants = { { RHI::ShaderStageFlags::Compute, 0, info.push_constant_size } };
		return ctx.create_compute_pipeline(desc);
	}

	RHI::GraphicsPipelineDesc desc{};
	desc.vertex_shader = program.get_stage_desc(RHI::ShaderStageFlags::Vertex);
	desc.fragment_shader = program.get_stage_desc(RHI::ShaderStageFlags::Fragment);
	desc.topology = RHI::PrimitiveTopology::TriangleList;
	desc.color_formats = info.color_formats;
	desc.depth_format = info.depth_format;

	desc.raster.cull_mode = info.cull_mode;
	desc.raster.front_face = info.front_face;

	desc.depth_stencil.depth_test = info.depth_test;
	desc.depth_stencil.depth_write = info.depth_write;

	if (info.blend_enabled) {
		RHI::BlendAttachmentDesc blend{};
		blend.enable = true;
		blend.src_color = RHI::BlendFactor::SrcAlpha;
		blend.dst_color = RHI::BlendFactor::OneMinusSrcAlpha;
		blend.color_op = RHI::BlendOp::Add;
		blend.src_alpha = RHI::BlendFactor::One;
		blend.dst_alpha = RHI::BlendFactor::Zero;
		blend.alpha_op = RHI::BlendOp::Add;
		desc.blend_attachments.assign(info.color_formats.size(), blend);
	}

	desc.set_layouts = { scene_layout };
	if (program.m_descriptor_set_layout) {
		desc.set_layouts.push_back(&program.m_descriptor_set_layout->get_rhi());
	}

	desc.push_constants = { { RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment, 0,
							  info.push_constant_size } };

	desc.debug_name = program.m_name;
	return ctx.create_graphics_pipeline(desc);
}

Ref<Material> MaterialFactory::create(GFX::GfxContext &ctx, const std::string &shader_path, MaterialCreateInfo info) {
	auto &entry = m_entries[shader_path];

	if (!entry.program) {
		entry.program = Ref<Shader::ShaderProgram>(new Shader::ShaderProgram(ctx, shader_path));
		entry.info = info;

		std::string err;
		if (!entry.program->add_stage_from_slang(shader_path, err)) {
			AQUILA_LOG_ERROR("MaterialFactory: compile failed for '{}': {}", shader_path, err);
			m_entries.erase(shader_path);
			return nullptr;
		}
		if (!entry.program->reflect()) {
			AQUILA_LOG_ERROR("MaterialFactory: reflection failed for '{}'", shader_path);
			m_entries.erase(shader_path);
			return nullptr;
		}

		entry.watch_id = Shader::ShaderHotReload::get()->register_reloadable(shader_path, [this, &ctx, shader_path]() {
			auto it = m_entries.find(shader_path);
			if (it != m_entries.end()) {
				rebuild_entry(ctx, it->second, shader_path);
			}
		});
	}

	auto pipeline = build_pipeline(ctx, *entry.program, info);
	if (!pipeline) {
		AQUILA_LOG_ERROR("MaterialFactory: pipeline creation failed for '{}'", shader_path);
		return nullptr;
	}

	Ref<Material> mat = Material::create_from_shader(ctx, *entry.program, pipeline);
	mat->set_type(info.type);
	mat->m_shader_path = shader_path;

	entry.instances.push_back(mat);
	return mat;
}

void MaterialFactory::rebuild_entry(GFX::GfxContext &ctx, Entry &entry, const std::string &shader_path) {
	std::string err;
	Ref<GFX::GfxDescriptorSetLayout> new_layout;

	if (!entry.program->reload(err, new_layout)) {
		AQUILA_LOG_ERROR("MaterialFactory: hot-reload compile failed for '{}': {}", shader_path, err);
		return;
	}

	auto new_pipeline = build_pipeline(ctx, *entry.program, entry.info);
	if (!new_pipeline) {
		AQUILA_LOG_ERROR("MaterialFactory: hot-reload pipeline failed for '{}'", shader_path);
		return;
	}

	entry.program->commit_new_layout(new_layout);

	ctx.wait_idle();

	auto it = entry.instances.begin();
	while (it != entry.instances.end()) {
		if (auto mat = it->lock()) {
			mat->replace_pipeline(new_pipeline, new_layout);
			++it;
		} else {
			it = entry.instances.erase(it);
		}
	}

	AQUILA_LOG_INFO("MaterialFactory: hot-reloaded '{}' ({} instances)", shader_path, entry.instances.size());
}

} // namespace Aquila::Graphics
