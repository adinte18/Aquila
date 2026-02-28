#pragma once

#include "Aquila/Graphics/Pipeline/RenderSettings.h"
#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Scene/Components/SkyLightComponent.h"
namespace Aquila::Rendering {
class Camera;
}

namespace Aquila::Utils {
class DeletionManager;
}

namespace Aquila::Rendering::Passes {
class ShadowPass;
}

namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics;
using namespace Aquila::Graphics::RenderingPipeline;
using namespace Aquila::Graphics::Resources;

struct PushConstants {
	uint32 lightCount = Defaults::MaxDirectionalLights;
};

struct LightData {
	vec3 color{ 1.0f };
	f32 intensity{ 1.0f };
	vec3 direction{ 0.0f, -1.0f, 0.0f };
	f32 range{ 10.0f };
	vec3 position{ 0.0f };
	int32 type{ 0 };
	f32 innerCone{ 0.0f };
	f32 outerCone{ 0.0f };
	int32 isActive{ 1 };
	f32 padding{ 0.0f };
};

struct LightsUniformData {
	LightData lights[Defaults::MaxDirectionalLights];
};

struct EnvironmentUniformData {
	alignas(16) vec4 shCoeffs[9]; // SH coefficients (RGB in xyz, w unused)
	alignas(16) vec3 tint;
	alignas(4) f32 intensity;
	alignas(4) int useEnvironment; // 0 = off, 1 = on
};

class LightingRenderSystem final : public RenderingSystemBase {
  public:
	LightingRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts);
	~LightingRenderSystem() override = default;

	void OnUpdate(const FrameSpec &frameSpec) override;
	void OnRender(const FrameSpec &frameSpec) override;
	void OnEvent(Events::Event &event) override;

	void SetEnvironment(const SceneManagement::Components::SHCoefficients &sh, f32 intensity = 1.0f,
						const vec3 &tint = vec3(1.0f));
	void EnableEnvironment(bool enabled) { m_EnvironmentEnabled = enabled; }
	bool IsEnvironmentEnabled() const { return m_EnvironmentEnabled; }

  protected:
	void CreatePipeline(const PipelineRenderingFormats &renderingFormats) override;
	void CreatePipelineLayout() override;

  private:
	void CreateFullscreenQuad();
	uint32 m_LightCount = 0;

	Unique<DescriptorSetLayout> m_GBufferLayout;
	Unique<DescriptorSetLayout> m_ShadowLayout;

	std::vector<Unique<Buffer>> m_LightsBuffers;
	std::vector<Unique<Buffer>> m_EnvironmentBuffers;

	SceneManagement::Components::SHCoefficients m_CurrentSH;
	f32 m_EnvironmentIntensity = 1.0f;
	vec3 m_EnvironmentTint = vec3(1.0f);
	bool m_EnvironmentEnabled = false;
	bool m_EnvironmentDirty = true;

	Unique<Buffer> m_QuadVertexBuffer;
	Unique<Buffer> m_QuadIndexBuffer;

	std::vector<VkDescriptorSet> m_GBufferDescriptorSets;
	std::vector<VkDescriptorSet> m_ShadowDescriptorSets;
};

} // namespace Aquila::Rendering::Systems
