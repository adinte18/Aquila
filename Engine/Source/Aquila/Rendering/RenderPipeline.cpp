#include "Aquila/Rendering/RenderPipeline.h"
#include "Aquila/RHI/Backend/RHITypes.h"
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

	const RenderView primary = resolve_primary_view(scene);
	{
		PROFILE_SCOPE("RenderPipeline::FrameDataUpdate");
		SceneFrameData::get()->update(scene, delta_time, m_frame_slot, primary);
	}

	FrameContext ctx;
	build_frame_context(scene, delta_time, primary, ctx);

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

RenderView RenderPipeline::resolve_primary_view(SceneManagement::Scene &scene) const {
	if (m_primary_view) {
		return *m_primary_view;
	}

	if (scene.has_active_camera()) {
		auto cam = scene.get_active_camera_entity();
		if (cam.has_all_components<CameraComponent, TransformComponent>()) {
			return render_view_from_entity(cam.get_component<CameraComponent>(),
										   cam.get_component<TransformComponent>());
		}
	}

	return RenderView{};
}

void RenderPipeline::build_frame_context(SceneManagement::Scene &scene, F32 delta_time, const RenderView &primary,
										 FrameContext &out) {
	out.scene = &scene;
	out.width = m_width;
	out.height = m_height;
	out.delta_time = delta_time;
	out.frame_data = SceneFrameData::get();
	out.frame_slot = m_frame_slot;

	out.h_scene_color = m_graph.import_texture(m_scene_color.get(), "SceneColor");
	out.h_depth = m_graph.import_texture(m_depth_tex.get(), "Depth");
	out.h_object_picking = m_graph.import_texture(m_object_picking.get(), "ObjectPicking");

	out.camera_position = primary.position;
	out.view = primary.view;
	out.projection = primary.projection;
	out.view_projection = primary.projection * primary.view;
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

	m_object_picking = m_ctx.create_texture({
		.width = m_width,
		.height = m_height,
		.format = RHI::TextureFormat::R32UI,
		.usage = RHI::TextureUsage::ColorAttachment | RHI::TextureUsage::TransferSrc,
		.debug_name = "ObjectPicking",
	});
}

} // namespace Aquila::Rendering
