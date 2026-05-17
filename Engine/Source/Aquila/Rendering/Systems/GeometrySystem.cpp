#include "Aquila/Rendering/Systems/GeometrySystem.h"
#include "Aquila/Rendering/FrameContext.h"
#include "Aquila/Rendering/SceneFrameData.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/Graphics/RenderGraph/RGGraph.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Foundation/Color.h"

namespace Aquila::Rendering {

using namespace SceneManagement::Components;
using namespace Graphics;

struct MeshPushConstants {
	mat4 model;
	vec4 color = vec4(1.F);
	uint32 materialIndex = 0;
};

void GeometrySystem::OnInit(GFX::GfxContext &ctx) {
	RenderingSystemBase::OnInit(ctx);
}

void GeometrySystem::AddPasses(RG::RenderGraph &graph, FrameContext &ctx) {
	auto &registry = ctx.scene->GetRegistry();
	auto view = registry.view<TransformComponent, MeshComponent>();

	struct DrawCall {
		Ref<GFX::GfxMesh> gpuMesh;
		mat4 model;
		uint32 materialIndex = 0;
	};

	std::unordered_map<Material *, std::vector<DrawCall>> batches;

	for (auto entity : view) {
		auto &transform = view.get<TransformComponent>(entity);
		auto &mesh = view.get<MeshComponent>(entity);
		if (!mesh.IsValid()) {
			continue;
		}

		auto *mat = registry.try_get<MaterialComponent>(entity);
		if (!mat || mat->type != MaterialType::Lit || !mat->material) {
			continue;
		}

		batches[mat->material.get()].push_back({
			.gpuMesh = GetOrUploadMesh(mesh.data),
			.model = transform.GetWorldMatrix(),
			.materialIndex = mat->materialIndex,
		});
	}

	if (batches.empty()) {
		return;
	}

	auto *frameData = ctx.frameData;
	const uint32 frameSlot = ctx.frameSlot;

	graph.AddPass(
		"Geometry",
		[&ctx](RG::RGPassBuilder &builder) {
			builder.ReadBuffer(ctx.hLightList, RG::ResourceState::ShaderRead);
			builder.ReadBuffer(ctx.hClusterLightInfo, RG::ResourceState::ShaderRead);

			ctx.hSceneColor =
				builder.SetColorAttachment(0, ctx.hSceneColor, RG::AttachmentLoadOp::Clear,
										   RG::AttachmentStoreOp::Store, { Foundation::Color::RGBA::DarkGray });

			builder.SetDepthAttachment(ctx.hDepth, RG::AttachmentLoadOp::Clear, RG::AttachmentStoreOp::Store,
									   RG::AttachmentLoadOp::DontCare, RG::AttachmentStoreOp::DontCare,
									   /*readOnly=*/false, RG::ClearDepth{ .depth = 1.F });
		},
		[batches = std::move(batches), frameData, frameSlot](GFX::GfxCommandList &cmd, RG::RGRegistry &) {
			for (auto &[material, drawCalls] : batches) {
				material->Flush();
				material->Bind(cmd, 1);
				cmd.BindDescriptorSet(0, frameData->GetDescriptorSet(frameSlot));

				for (const auto &dc : drawCalls) {
					MeshPushConstants push{ .model = dc.model, .materialIndex = dc.materialIndex };
					cmd.PushConstants(push, RHI::ShaderStageFlags::Vertex | RHI::ShaderStageFlags::Fragment);
					cmd.BindVertexBuffer(dc.gpuMesh->GetVertexBuffer());
					cmd.BindIndexBuffer(dc.gpuMesh->GetIndexBuffer());
					cmd.DrawIndexed(dc.gpuMesh->GetIndexCount());
				}
			}
		});
}

} // namespace Aquila::Rendering
