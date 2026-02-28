#include "Aquila/Rendering/Systems/DeferredRenderingSystem.h"

#include "Aquila/Core/Defines.h"
#include "Aquila/Events/Event.h"
#include "Aquila/Events/SceneEvent.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Material/MaterialSystem.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/EntityManager.h"

#include "Aquila/Utilities/Profiler.h"
#include "AquilaPCH.h"

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics::Helpers;

DeferredRenderSystem::DeferredRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts,
										   MaterialSystem &materialSystem)
	: RenderingSystemBase(device), m_MaterialSystem(materialSystem) {
	m_Layouts = layouts;
}

void DeferredRenderSystem::CreatePipeline(const PipelineRenderingFormats &renderingFormats) {
	// Material system handles pipeline creation now
}

void DeferredRenderSystem::CreatePipelineLayout() {
	// Material system handles this
}

void DeferredRenderSystem::DetectEntityChanges(const FrameSpec &frameSpec, SceneCache &cache,
											   SceneDescriptorData &sceneData) {
	for (auto &entity : cache.renderableEntities) {
		if (!entity.HasComponent<SceneManagement::Components::MeshComponent>()) {
			cache.entitiesDirty = true;
			return;
		}

		auto &meshComp = entity.GetComponent<SceneManagement::Components::MeshComponent>();
		UUID entityID = entity.GetUUID();
		auto &state = sceneData.entityStates[entityID];

		if (!HasEntityChanged(frameSpec.scene, entityID, meshComp, state)) {
			continue;
		}

		if (HasMaterialsChanged(meshComp, state)) {
			RefreshEntityMaterials(frameSpec, meshComp, state, cache);
		}

		if (HasMeshChanged(frameSpec.scene, entityID, meshComp)) {
			UpdateEntityState(frameSpec.scene, entityID, meshComp);
		}
	}
}
void DeferredRenderSystem::RefreshEntityMaterials(const FrameSpec &frameSpec,
												  const SceneManagement::Components::MeshComponent &mesh,
												  EntityRenderState &state, SceneCache &cache) {
	uint32 slotCount = std::max(mesh.GetMaterialSlotCount(), 1U);

	state.lastMaterials.clear();
	state.lastMaterials.reserve(slotCount);

	for (uint32 i = 0; i < slotCount; i++) {
		auto mat = mesh.GetMaterial(i);
		state.lastMaterials.push_back(mat);
		m_MaterialSystem.AllocateDescriptorsForMaterial(mat, frameSpec.scene);
	}

	cache.materialGroupsDirty = true;
}

void DeferredRenderSystem::RebuildEntityCacheIfDirty(const FrameSpec &frameSpec, SceneCache &cache) {
	if (!cache.entitiesDirty) {
		return;
	}

	cache.renderableEntities.clear();

	frameSpec.scene->GetEntityManager()
		->ForEach<SceneManagement::Components::MetadataComponent, SceneManagement::Components::MeshComponent>(
			[&](SceneManagement::Entity entity, const SceneManagement::Components::MetadataComponent &meta,
				SceneManagement::Components::MeshComponent &mesh) {
				if (meta.IsVisible() && mesh.IsValid()) {
					cache.renderableEntities.push_back(entity);
				}
			});

	cache.entitiesDirty = false;
	cache.materialGroupsDirty = true;
}

void DeferredRenderSystem::UpdateMaterials(const FrameSpec &frameSpec, SceneCache &cache,
										   SceneDescriptorData &sceneData) {
	m_MaterialSystem.TickHotReload(frameSpec.frameIndex);

	if (cache.materialGroupsDirty || sceneData.needsUpdate[frameSpec.frameIndex]) {
		UpdateDescriptorsForScene(frameSpec);
		m_MaterialSystem.UpdateAllDirtyMaterialsForScene(frameSpec.scene, frameSpec.frameIndex);
		sceneData.needsUpdate[frameSpec.frameIndex] = false;
	}
}

void DeferredRenderSystem::RebuildMaterialGroupsIfDirty(SceneCache &cache) {
	if (!cache.materialGroupsDirty) {
		return;
	}

	cache.materialGroups.clear();

	for (auto &entity : cache.renderableEntities) {
		auto &meshComp = entity.GetComponent<SceneManagement::Components::MeshComponent>();
		auto material = meshComp.GetRenderMaterial(0, m_MaterialSystem.GetLibrary().GetFallbackMaterial());

		if (!material) {
			AQUILA_LOG_ERROR("Entity has no material and fallback is null!");
			continue;
		}

		// find existing batch for this material
		auto it = std::find_if(cache.materialGroups.begin(), cache.materialGroups.end(),
							   [&](const SceneCache::MaterialBatch &b) { return b.material == material; });

		if (it != cache.materialGroups.end()) {
			it->entities.push_back(entity);
		} else {
			cache.materialGroups.push_back({ material, { entity } });
		}
	}

	cache.materialGroupsDirty = false;
}
void DeferredRenderSystem::DrawMaterialGroups(const FrameSpec &frameSpec, const SceneCache &cache) {
	for (const auto &batch : cache.materialGroups) {
		if (!batch.material || batch.entities.empty()) {
			continue;
		}
		if (batch.material->GetShader()->GetShaderType() != ShaderType::Standard) {
			continue;
		}

		if (!BindMaterial(frameSpec, batch.material)) {
			continue;
		}

		DrawEntities(frameSpec, batch.entities, batch.material->GetTemplate()->shader->m_Layout);
	}
}

// returns false if pipeline/layout are invalid — skips the draw
bool DeferredRenderSystem::BindMaterial(const FrameSpec &frameSpec, const Ref<Graphics::Material::Material> &material) {
	auto *pipeline = material->GetPipeline();
	if (pipeline == nullptr) {
		AQUILA_LOG_WARNING("Material '{}' has no pipeline", material->name);
		return false;
	}

	auto *pipelineLayout = material->GetTemplate()->shader->m_Layout;
	if (pipelineLayout == nullptr) {
		AQUILA_LOG_WARNING("Material '{}' has no pipeline layout", material->name);
		return false;
	}

	vkCmdBindPipeline(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// set 0 = shared camera (centralized UBO)
	// set 1 = material descriptors
	VkDescriptorSet matSet = m_MaterialSystem.GetMaterialDescriptorSet(material, frameSpec.scene, frameSpec.frameIndex);

	if (matSet != VK_NULL_HANDLE) {
		std::array<VkDescriptorSet, 2> sets = { frameSpec.cameraDescriptorSet, matSet };
		vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2,
								sets.data(), 0, nullptr);
	} else {
		vkCmdBindDescriptorSets(frameSpec.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
								&frameSpec.cameraDescriptorSet, 0, nullptr);
	}

	return true;
}

void DeferredRenderSystem::DrawEntities(const FrameSpec &frameSpec,
										const std::vector<SceneManagement::Entity> &entities,
										VkPipelineLayout pipelineLayout) {
	for (const auto &entity : entities) {
		const auto &mesh = entity.GetComponent<SceneManagement::Components::MeshComponent>();
		const auto &transform = entity.GetComponent<SceneManagement::Components::TransformComponent>();

		if (!mesh.IsValid()) {
			continue;
		}

		mesh.data->Bind(frameSpec.commandBuffer);

		DeferredPushConstants push{};
		push.modelMatrix = transform.GetWorldMatrix();
		push.normalMatrix = transform.GetNormalMatrix();

		vkCmdPushConstants(frameSpec.commandBuffer, pipelineLayout,
						   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DeferredPushConstants),
						   &push);

		mesh.data->Draw(frameSpec.commandBuffer);
	}
}
void DeferredRenderSystem::OnUpdate(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		return;
	}

	auto &cache = m_SceneCaches[frameSpec.scene];
	auto &sceneData = GetOrCreateSceneData(frameSpec.scene);

	DetectEntityChanges(frameSpec, cache, sceneData);
	RebuildEntityCacheIfDirty(frameSpec, cache);
	UpdateMaterials(frameSpec, cache, sceneData);
}

void DeferredRenderSystem::OnEvent(Events::Event &event) {
	Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Events::EntityVisibilityToggle>([this](Events::EntityVisibilityToggle &event) {
		auto *scene = event.GetEntity().GetScene();
		auto iterator = m_SceneCaches.find(scene);
		if (iterator == m_SceneCaches.end()) {
			return false;
		}

		auto &cache = iterator->second;

		if (event.IsVisibile()) {
			cache.renderableEntities.push_back(event.GetEntity());
		} else {
			std::erase(cache.renderableEntities, event.GetEntity());
		}

		cache.materialGroupsDirty = true;

		return true; // consumed
	});

	dispatcher.Dispatch<Events::EntityCreatedEvent>([this](Events::EntityCreatedEvent &event) {
		auto *scene = event.GetEntity().GetScene();
		auto iterator = m_SceneCaches.find(scene);
		if (iterator == m_SceneCaches.end()) {
			return false;
		}

		if (event.GetEntity()
				.HasAllComponents<SceneManagement::Components::MeshComponent,
								  SceneManagement::Components::TransformComponent>()) {
			iterator->second.renderableEntities.push_back(event.GetEntity());
			iterator->second.materialGroupsDirty = true;
		}
		return true; // consumed7
	});

	dispatcher.Dispatch<Events::EntityDeletedEvent>([this](Events::EntityDeletedEvent &event) {
		auto iterator = m_SceneCaches.find(event.GetEntity().GetScene());
		if (iterator != m_SceneCaches.end()) {
			auto &cache = iterator->second;
			std::erase(cache.renderableEntities, event.GetEntity());
			cache.materialGroupsDirty = true;
		}
		return true;
	});
}

void DeferredRenderSystem::OnRender(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		return;
	}

	auto &cache = m_SceneCaches[frameSpec.scene];
	if (cache.renderableEntities.empty()) {
		return;
	}

	RebuildMaterialGroupsIfDirty(cache);
	DrawMaterialGroups(frameSpec, cache);
}

} // namespace Aquila::Rendering::Systems
