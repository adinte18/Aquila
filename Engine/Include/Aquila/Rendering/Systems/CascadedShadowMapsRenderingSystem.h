#pragma once

#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "RenderingSystemBase.h"
#include "Aquila/Graphics/Pipeline/Pipeline.h"
#include "Aquila/Graphics/Resources/Buffer.h"

namespace Aquila::Rendering {
class Camera;
}

namespace Aquila::Foundation {
class DeletionManager;
}
namespace Aquila::Rendering::Systems {

using namespace Aquila::Graphics;
using namespace Aquila::Graphics::RenderingPipeline;
using namespace Aquila::Graphics::Resources;

struct alignas(16) ShadowUniformData {
	mat4 cascadeViewProj[4]; // 4 * 64 = 256 bytes
	alignas(16) vec4 cascadeSplits[4]; // 16 bytes (one vec4 for the 4 split distances)
	alignas(16) vec3 lightDirection; // 16 bytes (12 + pad)
	f32 lightSize; // Light size for penumbra (16 bytes with padding)
	f32 shadowBias; // Base shadow bias
	f32 normalBias; // Normal-based bias multiplier
	int pcfSamples; // Number of PCF samples
};

struct ShadowPushConstants {
	mat4 modelMatrix;
	uint32_t cascadeIndex;
};

class ShadowRenderSystem final : public RenderingSystemBase {
  public:
	ShadowRenderSystem(Device &device, const std::vector<Ref<DescriptorSetLayout>> &layouts);
	~ShadowRenderSystem() override = default;

	void OnUpdate(const FrameSpec &frameSpec) override;
	void OnRender(const FrameSpec &frameSpec) override;
	void OnEvent(Events::Event &event) override;

	const ShadowUniformData &GetShadowData() const { return m_ShadowData; }
	VkDescriptorBufferInfo GetShadowUniformBufferInfo(uint32 frameIndex) {
		return m_UniformBuffers[frameIndex]->DescriptorInfo();
	}

	SceneManagement::Components::ShadowQualitySettings &GetQualitySettings() { return m_QualitySettings; }
	const SceneManagement::Components::ShadowQualitySettings &GetQualitySettings() const { return m_QualitySettings; }
	void SetQualitySettings(const SceneManagement::Components::ShadowQualitySettings &settings) {
		m_QualitySettings = settings;
	}

	static constexpr uint32 CASCADE_COUNT = 4;
	static constexpr uint32 SHADOW_MAP_SIZE = 2048;

	Ref<RenderingPipeline::RenderTarget> GetCascadeRenderTarget(uint32 cascadeIndex) const {
		if (cascadeIndex >= m_CascadeRenderTargets.size()) {
			return nullptr;
		}
		return m_CascadeRenderTargets[cascadeIndex];
	}

	Ref<Texture2D> GetCascadeDepthAttachment(uint32 cascadeIndex) const {
		auto target = GetCascadeRenderTarget(cascadeIndex);
		return target ? target->GetDepthAttachment() : nullptr;
	}

	uint32 GetShadowMapSize() const { return SHADOW_MAP_SIZE; }

	VkDescriptorBufferInfo GetShadowUniformBufferInfo(int frameIndex) const {
		return m_UniformBuffers[frameIndex]->DescriptorInfo();
	}

  protected:
	void CreatePipeline(const PipelineRenderingFormats &renderingFormats) override;
	void CreatePipelineLayout() override;

  private:
	struct ShadowUpdateCache {
		vec3 lastLightDirection{ 0.0f, -1.0f, 0.0f };
		mat4 lastViewMatrix{ 1.0f };
		mat4 lastProjectionMatrix{ 1.0f };
		f32 lastNearPlane = 0.0f;
		f32 lastFarPlane = 0.0f;
		SceneManagement::Components::ShadowQualitySettings lastQualitySettings;

		uint32_t lastUpdateFrame = UINT32_MAX;
		bool needsUpdate = true;

		[[nodiscard]] bool HasCameraChanged(const mat4 &view, const mat4 &proj, f32 nearPlane, f32 farPlane,
											f32 epsilon = 0.001f) const {
			if (std::abs(lastNearPlane - nearPlane) > epsilon || std::abs(lastFarPlane - farPlane) > epsilon) {
				return true;
			}

			// Check view matrix change (position + rotation)
			for (int i = 0; i < 4; i++) {
				for (int j = 0; j < 4; j++) {
					if (std::abs(lastViewMatrix[i][j] - view[i][j]) > epsilon) {
						return true;
					}
				}
			}

			return false;
		}

		[[nodiscard]] bool HasLightChanged(const vec3 &lightDir, f32 epsilon = 0.001f) const {
			return length(lastLightDirection - lightDir) > epsilon;
		}

		[[nodiscard]] bool HasQualityChanged(const SceneManagement::Components::ShadowQualitySettings &settings) const {
			return std::memcmp(&lastQualitySettings, &settings, sizeof(settings)) != 0;
		}
	};

	std::unordered_map<SceneManagement::Scene *, ShadowUpdateCache> m_ShadowCaches;

	struct LightCache {
		vec3 direction{ 0.0f, -1.0f, 0.0f };
		bool hasDirectionalLight = false;
		SceneManagement::Components::ShadowQualitySettings qualitySettings;
		uint32_t lastCheckFrame = UINT32_MAX;
	};
	std::unordered_map<SceneManagement::Scene *, LightCache> m_LightCaches;

	mat4 GetLightSpaceMatrix(f32 nearPlane, f32 farPlane, const mat4 &view, const vec3 &lightDirection, f32 fov,
							 f32 aspectRatio);
	vec3 GetPrimaryDirectionalLightDirection(SceneManagement::Scene *scene);
	void CalculateCascadeSplits(f32 nearPlane, f32 farPlane);
	std::array<vec4, 8> GetFrustumCornersWorldSpace(const mat4 &projection, const mat4 &view);

	std::vector<Unique<Buffer>> m_UniformBuffers;
	ShadowUniformData m_ShadowData{};
	std::vector<f32> m_CascadeSplits;
	std::vector<Pipeline *> m_CascadePipelines;
	std::vector<Ref<RenderingPipeline::RenderTarget>> m_CascadeRenderTargets;

	SceneManagement::Components::ShadowQualitySettings m_QualitySettings;
};

} // namespace Aquila::Rendering::Systems
