#include "Aquila/Rendering/RenderPipeline.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"

namespace Aquila::Rendering {

using namespace SceneManagement::Components;

RenderPipeline::RenderPipeline(GFX::GfxContext &ctx, uint32 width, uint32 height)
	: m_Ctx(ctx), m_Width(width), m_Height(height) {
	RebuildTargets();
}

RenderPipeline::~RenderPipeline() {
	for (auto &r : m_Renderers) {
		r->OnShutdown();
	}
}

void RenderPipeline::Render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, f32 deltaTime) {
	scene.UpdateTransformHierarchy();

	FrameContext ctx;
	BuildFrameContext(scene, deltaTime, ctx);

	for (auto &renderer : m_Renderers) {
		renderer->AddPasses(m_Graph, ctx);
	}

	m_Graph.Compile(m_Ctx);
	m_Graph.Execute(cmd);
	m_Graph.Reset();
}
void RenderPipeline::Render(GFX::GfxCommandList &cmd, SceneManagement::Scene &scene, f32 deltaTime, uint32 width,
							uint32 height) {
	if (width != m_Width || height != m_Height) {
		Resize(width, height);
	}

	Render(cmd, scene, deltaTime);
}

void RenderPipeline::Resize(uint32 width, uint32 height) {
	m_Width = width;
	m_Height = height;
	RebuildTargets();
	for (auto &r : m_Renderers) {
		r->OnResize(width, height);
	}
}

void RenderPipeline::BuildFrameContext(SceneManagement::Scene &scene, f32 deltaTime, FrameContext &out) {
	out.scene = &scene;
	out.width = m_Width;
	out.height = m_Height;
	out.deltaTime = deltaTime;

	out.hSceneColor = m_Graph.ImportTexture(m_SceneColor.get(), "SceneColor");
	out.hDepth = m_Graph.ImportTexture(m_DepthTex.get(), "Depth");

	if (scene.HasActiveCamera()) {
		auto cam = scene.GetActiveCameraEntity();
		if (cam.HasAllComponents<CameraComponent, TransformComponent>()) {
			auto &camComp = cam.GetComponent<CameraComponent>();
			auto &transform = cam.GetComponent<TransformComponent>();

			out.cameraPosition = transform.GetWorldPosition();
			out.view = camComp.GetViewMatrix(out.cameraPosition, transform.GetLocalRotation());
			out.projection = camComp.GetProjectionMatrix();
			out.viewProjection = out.projection * out.view;
			return;
		}
	}

	out.view = out.projection = out.viewProjection = mat4(1.f);
}

void RenderPipeline::RebuildTargets() {
	m_SceneColor = m_Ctx.CreateTexture({
		.width = m_Width,
		.height = m_Height,
		.format = RHI::TextureFormat::RGBA16F,
		.usage = RHI::TextureUsage::ColorAttachment | RHI::TextureUsage::Sampled,
		.sampler = RHI::SamplerDesc::RenderTarget(),
		.debugName = "SceneColor",
	});

	m_DepthTex = m_Ctx.CreateTexture({
		.width = m_Width,
		.height = m_Height,
		.format = RHI::TextureFormat::Depth32,
		.usage = RHI::TextureUsage::DepthAttachment,
		.debugName = "Depth",
	});
}

} // namespace Aquila::Rendering
