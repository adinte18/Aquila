#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::SceneManagement {
class Scene;
}

namespace Aquila::Rendering {

class SceneFrameData : public Foundation::Singleton<SceneFrameData> {
  public:
	SceneFrameData(GFX::GfxContext &ctx, uint32 width, uint32 height);

	void Update(SceneManagement::Scene &scene, float deltaTime, uint32 frameSlot);

	void OnResize(uint32 width, uint32 height);

	[[nodiscard]] GFX::GfxDescriptorSet &GetDescriptorSet(uint32 frameSlot) const;
	[[nodiscard]] GFX::GfxDescriptorSetLayout &GetLayout() const { return *m_Layout; }

	[[nodiscard]] GFX::GfxBuffer &GetLightIndexListBuffer() const { return *m_LightIndexListBuffer; }
	[[nodiscard]] GFX::GfxBuffer &GetClusterLightInfoBuffer() const { return *m_ClusterLightInfoBuffer; }

  private:
	GFX::GfxContext &m_Ctx;
	uint32 m_Width = 0;
	uint32 m_Height = 0;
	uint32 m_FrameIndex = 0;
	float m_Time = 0.f;

	Ref<GFX::GfxDescriptorSetLayout> m_Layout;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_FrameBuffers;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_LightBuffers;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_EnvBuffers;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_MaterialBuffers;

	Ref<GFX::GfxBuffer> m_LightIndexListBuffer;

	Ref<GFX::GfxBuffer> m_ClusterLightInfoBuffer;

	std::array<Ref<GFX::GfxDescriptorSet>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_Sets;
};

} // namespace Aquila::Rendering
