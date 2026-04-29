// #ifndef DEFERREDRENDERSYSTEM_H
// #define DEFERREDRENDERSYSTEM_H

// #include "Aquila/Core/Defines.h"
// #include "Aquila/Graphics/Material/MaterialSystem.h"
// #include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
// #include "Aquila/Graphics/Resources/Buffer.h"
// #include "Aquila/Rendering/Systems/RenderingSystemBase.h"

// namespace Aquila::Rendering::Systems {

// using namespace Aquila::Graphics;
// using namespace Aquila::Graphics::RenderingPipeline;
// using namespace Aquila::Graphics::Material;
// using namespace Aquila::Graphics::Resources;

// struct DeferredPushConstants {
// 	mat4 modelMatrix{ 1.0F };
// 	mat4 normalMatrix{ 1.0F };
// };

// class DeferredRenderSystem final : public RenderingSystemBase {
//   public:
// 	DeferredRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts,
// 						 MaterialSystem &materialSystem);

// 	~DeferredRenderSystem() override = default;

// 	AQUILA_NONCOPYABLE(DeferredRenderSystem);
// 	AQUILA_NONMOVEABLE(DeferredRenderSystem);

// 	void OnUpdate(const FrameSpec &frameSpec) override;
// 	void OnRender(const FrameSpec &frameSpec) override;
// 	void OnEvent(Events::Event &event) override;

//   private:
// 	void RebuildMaterialGroupsIfDirty(SceneCache &cache);
// 	void DrawMaterialGroups(const FrameSpec &frameSpec, const SceneCache &cache);
// 	bool BindMaterial(const FrameSpec &frameSpec, const Ref<Graphics::Material::Material> &material);
// 	void DrawEntities(const FrameSpec &frameSpec, const std::vector<SceneManagement::Entity> &entities,
// 					  VkPipelineLayout pipelineLayout);
// 	void DetectEntityChanges(const FrameSpec &frameSpec, SceneCache &cache, SceneDescriptorData &sceneData);
// 	void UpdateMaterials(const FrameSpec &frameSpec, SceneCache &cache, SceneDescriptorData &sceneData);
// 	void RebuildEntityCacheIfDirty(const FrameSpec &frameSpec, SceneCache &cache);
// 	void RefreshEntityMaterials(const FrameSpec &frameSpec, const SceneManagement::Components::MeshComponent &mesh,
// 								EntityRenderState &state, SceneCache &cache);
// 	void CreatePipeline(const PipelineRenderingFormats &renderingFormats) override;
// 	void CreatePipelineLayout() override;
// 	MaterialSystem &m_MaterialSystem;

// 	std::vector<Unique<Buffer>> m_UniformBuffers;
// };

// } // namespace Aquila::Rendering::Systems

// #endif
