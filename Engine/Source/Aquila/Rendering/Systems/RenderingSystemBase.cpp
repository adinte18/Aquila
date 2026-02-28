#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Graphics/Core/Swapchain.h"

namespace Aquila::Rendering::Systems {

RenderingSystemBase::RenderingSystemBase(Device &device) : device(device) {}

RenderingSystemBase::~RenderingSystemBase() {
	m_SceneDescriptors.clear();

	if (m_PipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device.GetDevice(), m_PipelineLayout, nullptr);
		m_PipelineLayout = VK_NULL_HANDLE;
	}
}

SceneDescriptorData &RenderingSystemBase::GetOrCreateSceneData(SceneManagement::Scene *scene) {
	auto it = m_SceneDescriptors.find(scene);
	if (it == m_SceneDescriptors.end()) {
		auto [newIt, inserted] = m_SceneDescriptors.emplace(scene, SceneDescriptorData{});
		return newIt->second;
	}
	return it->second;
}

EntityRenderState &RenderingSystemBase::GetOrCreateEntityState(SceneManagement::Scene *scene, UUID entityID) {
	auto &sceneData = GetOrCreateSceneData(scene);
	return sceneData.entityStates[entityID];
}

bool RenderingSystemBase::HasEntityChanged(SceneManagement::Scene *scene, UUID entityID,
										   const SceneManagement::Components::MeshComponent &meshComp,
										   EntityRenderState &state) {
	return HasMeshChanged(scene, entityID, meshComp) || HasMaterialsChanged(meshComp, state);
}

bool RenderingSystemBase::HasMeshChanged(SceneManagement::Scene *scene, UUID entityID,
										 const SceneManagement::Components::MeshComponent &meshComp) {
	auto &state = GetOrCreateEntityState(scene, entityID);

	if (state.lastMeshVersion != meshComp.version) {
		return true;
	}

	if (state.isCastingShadows != meshComp.castShadows) {
		return true;
	}

	if (state.lastMesh != meshComp.data) {
		return true;
	}

	return false;
}

bool RenderingSystemBase::HasMaterialsChanged(const SceneManagement::Components::MeshComponent &mesh,
											  const EntityRenderState &state) {
	uint32 slotCount = std::max(mesh.GetMaterialSlotCount(), 1U);

	if (state.lastMaterials.size() != slotCount) {
		return true;
	}

	for (uint32 i = 0; i < slotCount; i++) {
		if (state.lastMaterials[i] != mesh.GetMaterial(i)) {
			return true;
		}
	}
	return false;
}

void RenderingSystemBase::UpdateEntityState(SceneManagement::Scene *scene, UUID entityID,
											const SceneManagement::Components::MeshComponent &meshComp) {
	auto &state = GetOrCreateEntityState(scene, entityID);

	if (state.lastMeshVersion != meshComp.version || state.lastMesh != meshComp.data) {
		state.lastMeshVersion = meshComp.version;
		state.lastMesh = meshComp.data;
		state.isCastingShadows = meshComp.castShadows;
		state.needsRebind = false;
	}

	state.isCastingShadows = meshComp.castShadows;

	AQUILA_LOG_INFO("Updated entity state");
}

void RenderingSystemBase::AllocateDescriptorSetsForScene(SceneManagement::Scene *scene) {
	if (m_Layouts.empty()) {
		AQUILA_LOG_WARNING("No descriptor set layouts, skipping allocation for scene '{}'", scene->GetSceneName());
		return;
	}

	auto &sceneData = GetOrCreateSceneData(scene);

	sceneData.descriptorSets.clear();
	sceneData.descriptorSets.resize(m_Layouts.size());

	auto pool = DescriptorAllocator::GetSharedPool();
	if (!pool) {
		AQUILA_LOG_ERROR("No descriptor pool available");
		return;
	}

	for (uint32 setIndex = 0; setIndex < m_Layouts.size(); ++setIndex) {
		sceneData.descriptorSets[setIndex].resize(SharedConstants::MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

		for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
			if (!pool->AllocateDescriptor(m_Layouts[setIndex]->GetDescriptorSetLayout(),
										  sceneData.descriptorSets[setIndex][frame])) {
				AQUILA_LOG_ERROR("Failed to allocate descriptor set {} for frame {} in scene '{}'", setIndex, frame,
								 scene->GetSceneName());
			} else {
				AQUILA_LOG_DEBUG("Allocated descriptor set {} for frame {} in scene '{}': {}", setIndex, frame,
								 scene->GetSceneName(), static_cast<void *>(sceneData.descriptorSets[setIndex][frame]));
			}
			sceneData.needsUpdate[frame] = true;
		}
	}
}

void RenderingSystemBase::BindUniformBuffer(SceneManagement::Scene *scene, uint32 set, uint32 binding,
											const VkDescriptorBufferInfo *bufferInfo) {
	if (scene == nullptr) {
		AQUILA_LOG_ERROR("Cannot bind uniform buffer: scene is null");
		return;
	}

	auto &sceneData = GetOrCreateSceneData(scene);
	sceneData.pendingUniformBuffers[set][binding] = *bufferInfo;
	for (int frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; frame++) {
		sceneData.needsUpdate[frame] = true;
	}
}

void RenderingSystemBase::BindTexture(SceneManagement::Scene *scene, uint32 set, uint32 binding,
									  const VkDescriptorImageInfo *imageInfo) {
	if (!scene) {
		AQUILA_LOG_ERROR("Cannot bind texture: scene is null");
		return;
	}

	auto &sceneData = GetOrCreateSceneData(scene);
	sceneData.pendingTextures[set][binding] = *imageInfo;
	for (int frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; frame++) {
		sceneData.needsUpdate[frame] = true;
	}
}

void RenderingSystemBase::BindTexture(SceneManagement::Scene *scene, uint32 set, uint32 binding, Texture2D *texture) {
	if (texture != nullptr) {
		auto imageInfo = texture->GetDescriptorImageInfo();
		BindTexture(scene, set, binding, &imageInfo);
	}
}

void RenderingSystemBase::BindTextureArray(SceneManagement::Scene *scene, uint32 set, uint32 binding,
										   const VkDescriptorImageInfo *imageInfo) {
	if (scene == nullptr) {
		AQUILA_LOG_ERROR("Cannot bind texture array: scene is null");
		return;
	}

	auto &sceneData = GetOrCreateSceneData(scene);
	sceneData.pendingTextureArrays[set][binding] = *imageInfo;
	for (int frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; frame++) {
		sceneData.needsUpdate[frame] = true;
	}
}

void RenderingSystemBase::UpdateDescriptorsForScene(const FrameSpec &frameSpec) {
	if (frameSpec.scene == nullptr) {
		AQUILA_LOG_WARNING("No scene in FrameSpec for descriptor update");
		return;
	}

	auto it = m_SceneDescriptors.find(frameSpec.scene);
	if (it == m_SceneDescriptors.end()) {
		AQUILA_LOG_WARNING("No descriptor data for scene '{}'", frameSpec.scene->GetSceneName());
		return;
	}

	auto &sceneData = it->second;

	if (!sceneData.needsUpdate[frameSpec.frameIndex]) {
		return;
	}

	auto pool = DescriptorAllocator::GetSharedPool();
	if (!pool) {
		AQUILA_LOG_ERROR("No descriptor pool available");
		return;
	}

	uint32 frameIndex = frameSpec.frameIndex;

	for (uint32 setIndex = 0; setIndex < m_Layouts.size(); ++setIndex) {
		if (setIndex >= sceneData.descriptorSets.size()) {
			continue;
		}

		if (frameIndex >= sceneData.descriptorSets[setIndex].size()) {
			continue;
		}

		VkDescriptorSet descriptorSet = sceneData.descriptorSets[setIndex][frameIndex];
		if (descriptorSet == VK_NULL_HANDLE) {
			continue;
		}

		DescriptorWriter writer{ *m_Layouts[setIndex], *pool };

		if (sceneData.pendingUniformBuffers.contains(setIndex)) {
			for (const auto &[binding, bufferInfo] : sceneData.pendingUniformBuffers[setIndex]) {
				writer.WriteBuffer(binding, const_cast<VkDescriptorBufferInfo *>(&bufferInfo));
			}
		}

		if (sceneData.pendingTextures.contains(setIndex)) {
			for (const auto &[binding, imageInfo] : sceneData.pendingTextures[setIndex]) {
				writer.WriteImage(binding, const_cast<VkDescriptorImageInfo *>(&imageInfo));
			}
		}

		// if (sceneData.pendingTextureArrays.find(setIndex) != sceneData.pendingTextureArrays.end()) {
		// 	for (const auto &[binding, imageInfo] : sceneData.pendingTextureArrays[setIndex]) {
		// 		writer.WriteImage(binding, const_cast<VkDescriptorImageInfo *>(&imageInfo));
		// 	}
		// }

		writer.Overwrite(descriptorSet);
	}

	sceneData.needsUpdate[frameSpec.frameIndex] = false;
}

VkDescriptorSet RenderingSystemBase::GetSceneDescriptorSet(SceneManagement::Scene *scene, uint32 setIndex,
														   uint32 frameIndex) const {
	auto it = m_SceneDescriptors.find(scene);
	if (it == m_SceneDescriptors.end()) {
		return VK_NULL_HANDLE;
	}

	const auto &sceneData = it->second;

	if (setIndex >= sceneData.descriptorSets.size()) {
		return VK_NULL_HANDLE;
	}

	if (frameIndex >= sceneData.descriptorSets[setIndex].size()) {
		return VK_NULL_HANDLE;
	}

	return sceneData.descriptorSets[setIndex][frameIndex];
}

void RenderingSystemBase::CleanupSceneDescriptors(SceneManagement::Scene *scene) {
	auto it = m_SceneDescriptors.find(scene);
	if (it != m_SceneDescriptors.end()) {
		AQUILA_LOG_DEBUG("Cleaning up descriptor sets for scene '{}'", scene->GetSceneName());

		AQUILA_LOG_DEBUG("Cleaned up {} entity states for scene '{}'", it->second.entityStates.size(),
						 scene->GetSceneName());

		m_SceneDescriptors.erase(it);
	}
}

void RenderingSystemBase::MarkSceneDescriptorsDirty(SceneManagement::Scene *scene) {
	auto it = m_SceneDescriptors.find(scene);
	if (it != m_SceneDescriptors.end()) {
		it->second.needsUpdate.fill(true);
	}
}
} // namespace Aquila::Rendering::Systems
