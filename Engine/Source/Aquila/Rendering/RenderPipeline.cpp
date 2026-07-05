#include "Aquila/Rendering/RenderPipeline.h"
#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/Foundation/Profiler.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"

namespace Aquila::Rendering {

using namespace SceneManagement::Components;

RenderPipeline::RenderPipeline(GFX::GfxContext &ctx, Uint32 width, Uint32 height)
	: m_ctx(ctx), m_width(width), m_height(height) {
	SceneFrameData::init(ctx, width, height);
	rebuild_targets();
}

RenderPipeline::~RenderPipeline() {
	for (auto &r : m_renderers) {
		r->on_shutdown();
	}
	SceneFrameData::shutdown();
}

void RenderPipeline::render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, F32 delta_time) {
	{
		PROFILE_SCOPE("RenderPipeline::UpdateTransforms");
		scene.update_transform_hierarchy();
	}

	m_frame_slot = (m_frame_slot + 1) % SharedConstants::MAX_FRAMES_IN_FLIGHT;
	{
		PROFILE_SCOPE("RenderPipeline::FrameDataUpdate");
		SceneFrameData::get()->update(scene, delta_time, m_frame_slot);
	}

	FrameContext ctx;
	build_frame_context(scene, delta_time, ctx);

	{
		PROFILE_SCOPE("RenderPipeline::AddPasses");
		for (auto &renderer : m_renderers) {
			renderer->add_passes(m_graph, ctx);
		}
	}

	{
		PROFILE_SCOPE("RenderPipeline::AddFinalPasses");
		for (auto &renderer : m_renderers) {
			renderer->blit_to_swapchain(m_graph, ctx);
		}
	}

	{
		PROFILE_SCOPE("RenderPipeline::GraphCompile");
		m_graph.compile(m_ctx);
	}
	{
		PROFILE_SCOPE("RenderPipeline::GraphExecute");
		m_graph.execute(cmd);
	}
	m_graph.reset();
}

void RenderPipeline::render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, F32 delta_time, Uint32 width,
							Uint32 height) {
	if (width != m_width || height != m_height) {
		resize(width, height);
	}
	render(cmd, scene, delta_time);
}

void RenderPipeline::resize(Uint32 width, Uint32 height) {
	m_width = width;
	m_height = height;
	SceneFrameData::get()->on_resize(width, height);
	rebuild_targets();
	for (auto &r : m_renderers) {
		r->on_resize(width, height);
	}
}

void RenderPipeline::build_frame_context(SceneManagement::Scene &scene, F32 delta_time, FrameContext &out) {
	out.scene = &scene;
	out.width = m_width;
	out.height = m_height;
	out.delta_time = delta_time;
	out.frame_data = SceneFrameData::get();
	out.frame_slot = m_frame_slot;

	out.h_scene_color = m_graph.import_texture(m_scene_color.get(), "SceneColor");
	out.h_depth = m_graph.import_texture(m_depth_tex.get(), "Depth");

	if (scene.has_active_camera()) {
		auto cam = scene.get_active_camera_entity();
		if (cam.has_all_components<CameraComponent, TransformComponent>()) {
			auto &cam_comp = cam.get_component<CameraComponent>();
			auto &transform = cam.get_component<TransformComponent>();

			out.camera_position = transform.get_world_position();
			out.view = cam_comp.get_view_matrix(out.camera_position, transform.get_local_rotation());
			out.projection = cam_comp.get_projection_matrix();
			out.view_projection = out.projection * out.view;
			return;
		}
	}

	out.view = out.projection = out.view_projection = Mat4(1.F);
}

void RenderPipeline::rebuild_targets() {
	m_scene_color.reset();
	m_depth_tex.reset();

	m_scene_color = m_ctx.create_texture({
		.width = m_width,
		.height = m_height,
		.format = RHI::TextureFormat::RGBA16F,
		.usage = RHI::TextureUsage::ColorAttachment | RHI::TextureUsage::Sampled,
		.sampler = RHI::SamplerDesc::render_target(),
		.debug_name = "SceneColor",
	});

	m_depth_tex = m_ctx.create_texture({
		.width = m_width,
		.height = m_height,
		.format = RHI::TextureFormat::Depth32,
		.usage = RHI::TextureUsage::DepthAttachment,
		.debug_name = "Depth",
	});
}

} // namespace Aquila::Rendering
