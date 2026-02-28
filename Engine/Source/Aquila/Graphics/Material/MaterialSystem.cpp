#include "Aquila/Graphics/Material/MaterialSystem.h"

#include "Aquila/Core/Defines.h"
#include "Aquila/Graphics/Core/Swapchain.h"
#include "Aquila/Graphics/Pipeline/Descriptor.h"
#include "Aquila/Graphics/Pipeline/DescriptorAllocator.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"

namespace Aquila::Graphics::Material {

// TODO : remove redundant frameIndexes all over the place

using namespace Aquila::Graphics::RenderingPipeline;

// SCENE MATERIAL DATA

SceneMaterialData &MaterialSystem::GetOrCreateSceneMaterialData(SceneManagement::Scene *scene) {
	auto iterator = m_SceneMaterialData.find(scene);
	if (iterator == m_SceneMaterialData.end()) {
		auto [newIt, inserted] = m_SceneMaterialData.emplace(scene, SceneMaterialData{});
		return newIt->second;
	}
	return iterator->second;
}

// DESCRIPTOR MANAGEMENT

void MaterialSystem::AllocateDescriptorsForMaterial(const Ref<Material> &material, SceneManagement::Scene *scene) {
	if (!material || (scene == nullptr)) {
		return;
	}

	auto &sceneData = GetOrCreateSceneMaterialData(scene);

	if (sceneData.materialDescriptorSets.contains(material->name)) {
		sceneData.dirtyMaterials.insert(material->name);
		sceneData.needsUpdate = true;
		return;
	}

	auto pool = DescriptorAllocator::GetSharedPool();

	if (!pool) {
		AQUILA_LOG_ERROR("No descriptor pool available for scene '{}'", scene->GetSceneName());
		return;
	}

	auto shader = material->GetTemplate()->shader;
	if (!shader || !shader->m_DescriptorSetLayout ||
		shader->m_DescriptorSetLayout->GetDescriptorSetLayout() == VK_NULL_HANDLE) {
		AQUILA_LOG_WARNING("Material '{}' shader has no descriptor set layout", material->name);
		return;
	}

	sceneData.materialDescriptorSets[material->name].resize(SharedConstants::MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

	for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		if (!pool->AllocateDescriptor(shader->m_DescriptorSetLayout->GetDescriptorSetLayout(), descriptorSet)) {
			AQUILA_LOG_ERROR("Failed to allocate descriptor for material '{}' frame {} in scene '{}'", material->name,
							 frame, scene->GetSceneName());
		} else {
			sceneData.materialDescriptorSets[material->name][frame] = descriptorSet;
			AQUILA_LOG_DEBUG("Allocated descriptor for material '{}' frame {} in scene '{}'", material->name, frame,
							 scene->GetSceneName());
		}
	}

	sceneData.dirtyMaterials.insert(material->name);
	sceneData.needsUpdate = true;

	material->CreateMaterialUBO();

	material->MarkDescriptorDirty();
}

void MaterialSystem::AllocateDescriptorsForMaterialAllScenes(const Ref<Material> &material) {
	for (auto &[scene, sceneData] : m_SceneMaterialData) {
		AllocateDescriptorsForMaterial(material, scene);
	}
}

void MaterialSystem::AllocateGlobalMaterialDescriptors(const Ref<Material> &material) {
	if (!material) {
		return;
	}

	auto shader = material->GetTemplate()->shader;
	if (!shader || !shader->m_DescriptorSetLayout) {
		return;
	}

	auto pool = DescriptorAllocator::GetSharedPool();

	m_GlobalMaterialDescriptors[material->name].resize(SharedConstants::MAX_FRAMES_IN_FLIGHT);

	for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		if (pool->AllocateDescriptor(shader->m_DescriptorSetLayout->GetDescriptorSetLayout(), descriptorSet)) {
			m_GlobalMaterialDescriptors[material->name][frame] = descriptorSet;
			AQUILA_LOG_INFO("Allocated global descriptor for engine material '{}' frame {}", material->name, frame);
		}
	}

	for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
		UpdateGlobalMaterialDescriptor(material, frame);
	}
}

VkDescriptorSet MaterialSystem::GetMaterialDescriptorSet(const Ref<Material> &material, SceneManagement::Scene *scene,
														 uint32 frameIndex) {
	if (!material) {
		return VK_NULL_HANDLE;
	}

	// check scene-local first
	if (scene != nullptr) {
		auto sceneIt = m_SceneMaterialData.find(scene);
		if (sceneIt != m_SceneMaterialData.end()) {
			auto matIt = sceneIt->second.materialDescriptorSets.find(material->name);
			if (matIt != sceneIt->second.materialDescriptorSets.end() && frameIndex < matIt->second.size() &&
				matIt->second[frameIndex] != VK_NULL_HANDLE) {
				return matIt->second[frameIndex];
			}
		}
	}

	// fall back to global
	auto globalIt = m_GlobalMaterialDescriptors.find(material->name);
	if (globalIt != m_GlobalMaterialDescriptors.end() && frameIndex < globalIt->second.size()) {
		return globalIt->second[frameIndex];
	}

	return VK_NULL_HANDLE;
}

void MaterialSystem::CleanupSceneMaterialData(SceneManagement::Scene *scene) {
	auto iterator = m_SceneMaterialData.find(scene);
	if (iterator != m_SceneMaterialData.end()) {
		AQUILA_LOG_DEBUG("Cleaning up material descriptors for scene '{}'", scene->GetSceneName());
		m_SceneMaterialData.erase(iterator);
	}
}

static void WriteDescriptorForMaterial(const Ref<Material> &material, VkDescriptorSet descriptorSet,
									   MaterialLibrary &library) {
	const auto tmpl = material->GetTemplate();

	DescriptorWriter writer(*tmpl->shader->m_DescriptorSetLayout, *DescriptorAllocator::GetSharedPool());
	const auto &shaderBindings = tmpl->shader->m_DescriptorSetLayout->GetBindings();

	std::map<int, std::pair<std::string, MaterialParameter>> textureBindings;
	for (const auto &[name, prop] : tmpl->properties) {
		if (prop.m_Type == ParameterType::Texture2D && prop.m_BindingIndex >= 0 &&
			shaderBindings.contains(prop.m_BindingIndex)) {
			textureBindings[prop.m_BindingIndex] = { name, prop };
		}
	}

	std::vector<VkDescriptorImageInfo> imageInfos;
	imageInfos.reserve(textureBindings.size());

	for (const auto &[binding, namePropPair] : textureBindings) {
		const auto &[name, prop] = namePropPair;
		auto texture = material->GetTexture(name);
		if (!texture) {
			texture = library.GetFallbackTextureForType(name);
		}
		if (texture) {
			imageInfos.push_back(texture->GetDescriptorImageInfo());
			writer.WriteImage(binding, &imageInfos.back());
		}
	}

	if (material->HasAnyUBO()) {
		size_t uboCount = 0;
		for (const auto &[bindingIdx, layoutBinding] : shaderBindings) {
			if (layoutBinding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
				++uboCount;
			}
		}

		std::vector<VkDescriptorBufferInfo> bufferInfos;
		bufferInfos.reserve(uboCount); // prevent realloc so pointers are alive in write buffer

		for (const auto &[bindingIdx, layoutBinding] : shaderBindings) {
			if (layoutBinding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
				continue;
			}
			auto *ubo = material->GetUBOBuffer(bindingIdx);
			AQUILA_LOG_DEBUG("Material '{}': binding {} UBO={}, VkBuffer={}", material->name, bindingIdx, (void *)ubo,
							 ubo ? (void *)ubo->GetBuffer() : nullptr);

			if (ubo == nullptr) {
				AQUILA_LOG_ERROR("Material '{}': no UBO buffer for binding {}", material->name, bindingIdx);
				continue;
			}
			if (ubo->GetBuffer() == VK_NULL_HANDLE) {
				continue;
			}
			bufferInfos.push_back(ubo->DescriptorInfo());
			writer.WriteBuffer(bindingIdx, &bufferInfos.back());
		}
	}

	writer.Overwrite(descriptorSet);
}

void MaterialSystem::UpdateDescriptorSetForScene(const Ref<Material> &material, SceneManagement::Scene *scene,
												 uint32 frameIndex) {
	if (!material || (scene == nullptr)) {
		return;
	}

	const auto tmpl = material->GetTemplate();
	if (!tmpl || !tmpl->shader || !tmpl->shader->m_DescriptorSetLayout) {
		return;
	}

	auto &sceneData = GetOrCreateSceneMaterialData(scene);
	auto iterator = sceneData.materialDescriptorSets.find(material->name);

	if (iterator == sceneData.materialDescriptorSets.end() || frameIndex >= iterator->second.size()) {
		AQUILA_LOG_ERROR("No descriptor set allocated for material '{}' frame {} in scene '{}'", material->name,
						 frameIndex, scene->GetSceneName());
		return;
	}

	VkDescriptorSet descriptorSet = iterator->second[frameIndex];
	if (descriptorSet == VK_NULL_HANDLE) {
		AQUILA_LOG_ERROR("Descriptor set is null for material '{}' frame {} in scene '{}'", material->name, frameIndex,
						 scene->GetSceneName());
		return;
	}

	WriteDescriptorForMaterial(material, descriptorSet, m_Library);
}

void MaterialSystem::UpdateGlobalMaterialDescriptor(const Ref<Material> &material, uint32 frameIndex) {
	if (!material) {
		return;
	}

	const auto tmpl = material->GetTemplate();
	if (!tmpl || !tmpl->shader || !tmpl->shader->m_DescriptorSetLayout) {
		AQUILA_LOG_ERROR("Material '{}' has invalid template or shader", material->name);
		return;
	}

	auto iterator = m_GlobalMaterialDescriptors.find(material->name);
	if (iterator == m_GlobalMaterialDescriptors.end() || frameIndex >= iterator->second.size()) {
		AQUILA_LOG_ERROR("No global descriptor set allocated for material '{}' frame {}", material->name, frameIndex);
		return;
	}

	VkDescriptorSet descriptorSet = iterator->second[frameIndex];
	if (descriptorSet == VK_NULL_HANDLE) {
		AQUILA_LOG_ERROR("Global descriptor set is null for material '{}' frame {}", material->name, frameIndex);
		return;
	}

	WriteDescriptorForMaterial(material, descriptorSet, m_Library);
	AQUILA_LOG_DEBUG("Updated global descriptor for material '{}' frame {}", material->name, frameIndex);
}

// UPDATE LOOP

void MaterialSystem::UpdateMaterialForScene(const Ref<Material> &material, SceneManagement::Scene *scene,
											uint32 frameIndex) {
	if (!material || !material->IsDirty()) {
		return;
	}

	if (material->IsDescriptorDirty()) {
		UpdateDescriptorSetForScene(material, scene, frameIndex);
	}

	if (material->IsPipelineDirty()) {
		UpdatePipeline(material, frameIndex);
	}
}

void MaterialSystem::UpdateAllDirtyMaterialsForScene(SceneManagement::Scene *scene, uint32 frameIndex) {
	if (scene == nullptr) {
		return;
	}

	auto &sceneData = GetOrCreateSceneMaterialData(scene);

	for (const auto &mat : m_Library.GetAllMaterials() | std::views::values) {
		if (!mat || mat->IsEngineMaterial()) {
			continue;
		}

		if (mat->IsDescriptorDirty() || sceneData.dirtyMaterials.contains(mat->name)) {
			for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
				UpdateDescriptorSetForScene(mat, scene, frame);
			}
			sceneData.dirtyMaterials.erase(mat->name);
			mat->MarkDescriptorClean();
		}

		if (mat->IsPipelineDirty()) {
			UpdatePipeline(mat, frameIndex);
			mat->MarkPipelineClean();
		}

		if (!mat->IsPipelineDirty() && !mat->IsDescriptorDirty()) {
			mat->MarkClean();
		}
	}

	UpdateGlobalEngineMaterials(frameIndex);
}

void MaterialSystem::UpdateGlobalEngineMaterials(uint32 frameIndex) {
	auto defaultMat = m_Library.GetDefaultMaterial();
	auto fallbackMat = m_Library.GetFallbackMaterial();

	if (defaultMat && defaultMat->IsDescriptorDirty()) {
		for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
			UpdateGlobalMaterialDescriptor(defaultMat, frame);
		}
		defaultMat->MarkDescriptorClean();
	}

	if (fallbackMat && fallbackMat->IsDescriptorDirty()) {
		for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
			UpdateGlobalMaterialDescriptor(fallbackMat, frame);
		}
		fallbackMat->MarkDescriptorClean();
	}
}

// HOT RELOAD

void MaterialSystem::TickHotReload(uint32 frameIndex) {
	m_Library.TickHotReload();
}

// PIPELINE MANAGEMENT

struct PushConstantData {
	glm::mat4 modelMatrix{ 1.F };
	glm::mat4 normalMatrix{ 1.F };
};

void MaterialSystem::BuildShaderMaterialMapping() {
	m_ShaderToMaterials.clear();
	for (const auto &mat : m_Library.GetAllMaterials() | std::views::values) {
		if (auto shader = mat->GetTemplate()->shader) {
			m_ShaderToMaterials[shader->m_Name].push_back(mat->name);
		}
	}
}

void MaterialSystem::UpdatePipeline(const Ref<Material> &material, uint32 frameIndex) {
	const auto &formats = material->GetShader()->GetTargetFormats();
	AQUILA_ASSERT(!formats.colorFormats.empty() || formats.depthFormat != VK_FORMAT_UNDEFINED,
				  "A shader has no target formats set — call SetTargetFormats() BEFORE creating materials");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfig(pipelineConfig);

	auto shader = material->GetTemplate()->shader;
	if (!shader || !shader->m_DescriptorSetLayout) {
		AQUILA_LOG_ERROR("Material '{}' has invalid shader or descriptor layout", material->name);
		return;
	}

	if (shader->m_Layout == VK_NULL_HANDLE) {
		shader->CreatePipelineLayout(m_GlobalLayout->GetDescriptorSetLayout());
	}

	pipelineConfig.colorFormats = formats.colorFormats;
	pipelineConfig.depthFormat = formats.depthFormat;
	pipelineConfig.stencilFormat = VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo renderingCreateInfo{};
	renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	renderingCreateInfo.colorAttachmentCount = static_cast<uint32>(pipelineConfig.colorFormats.size());
	renderingCreateInfo.pColorAttachmentFormats =
		pipelineConfig.colorFormats.empty() ? nullptr : pipelineConfig.colorFormats.data();
	renderingCreateInfo.depthAttachmentFormat = pipelineConfig.depthFormat;
	renderingCreateInfo.stencilAttachmentFormat = pipelineConfig.stencilFormat;

	pipelineConfig.pNext = &renderingCreateInfo;
	pipelineConfig.pipelineLayout = shader->m_Layout;

	if (VkPipeline old = material->GetPipeline(); old != VK_NULL_HANDLE) {
		material->SetPipeline(VK_NULL_HANDLE);
	}

	auto pipeline = CreateUnique<Pipeline>(m_Device, material, pipelineConfig);
	material->SetPipeline(pipeline->GetPipeline());
	m_Pipelines[material->name] = std::move(pipeline);

	AQUILA_LOG_DEBUG("Created new pipeline for material: {}", material->name);
}

void MaterialSystem::Initialize(DescriptorSetLayout *layout) {
	m_GlobalLayout = layout;

	m_Library.Initialize();

	m_Library.SetShaderReloadedCallback([this](const Ref<Material> &material) {
		AQUILA_LOG_INFO("Pipeline rebuild triggered for material '{}' after shader reload", material->name);

		// might have lost UBOs in the process so better to just delete
		material->DestroyMaterialUBO();
		// recreate ubo
		material->CreateMaterialUBO();

		UpdatePipeline(material, 0);

		for (auto &[scene, sceneData] : m_SceneMaterialData) {
			sceneData.materialDescriptorSets.erase(material->name);
			AllocateDescriptorsForMaterial(material, scene);
			sceneData.dirtyMaterials.insert(material->name);
			sceneData.needsUpdate = true;
		}

		// force descriptor update with them fresshhhhhhhh ubo's
		for (auto &[scene, sceneData] : m_SceneMaterialData) {
			for (uint32 frame = 0; frame < SharedConstants::MAX_FRAMES_IN_FLIGHT; ++frame) {
				UpdateDescriptorSetForScene(material, scene, frame);
			}
		}

		material->MarkDescriptorClean();
		material->MarkPipelineClean();
	});

	m_Library.SetMaterialCreatedCallback(
		[this](const Ref<Material> &material) { AllocateDescriptorsForMaterialAllScenes(material); });

	auto pool = DescriptorAllocator::GetSharedPool();
	if (pool) {
		AllocateGlobalMaterialDescriptors(m_Library.GetDefaultMaterial());
		AllocateGlobalMaterialDescriptors(m_Library.GetFallbackMaterial());
	}

	for (const auto &mat : m_Library.GetAllMaterials() | std::views::values) {
		if (mat->IsPipelineDirty()) {
			UpdatePipeline(mat, 0);
		}
	}

	BuildShaderMaterialMapping();
}

void MaterialSystem::Shutdown() {
	m_SceneMaterialData.clear();
	m_Pipelines.clear();
	m_Library.Shutdown();
	AQUILA_LOG_DEBUG("MaterialSystem shutdown complete");
}

MaterialLibrary &MaterialSystem::GetLibrary() {
	return m_Library;
}

} // namespace Aquila::Graphics::Material
