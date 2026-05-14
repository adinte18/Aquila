#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Rendering/FrameData.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::SceneManagement {
class Scene;
}

namespace Aquila::Rendering {

//
// GPU binding convention:
//   [vk::binding(0, 0)] ConstantBuffer<AqFrameData> aqFrameData;
//
class SceneFrameData : public Foundation::Singleton<SceneFrameData> {
  public:
	SceneFrameData(GFX::GfxContext &ctx, uint32 width, uint32 height);

	void Update(SceneManagement::Scene &scene, float deltaTime, uint32 frameSlot);

	void OnResize(uint32 width, uint32 height);

	[[nodiscard]] GFX::GfxDescriptorSet &GetDescriptorSet(uint32 frameSlot) const;

	[[nodiscard]] GFX::GfxDescriptorSetLayout &GetLayout() const { return *m_Layout; }

  private:
	GFX::GfxContext &m_Ctx;
	uint32 m_Width = 0;
	uint32 m_Height = 0;
	uint32 m_FrameIndex = 0; // monotonically increasing frame counter
	float m_Time = 0.f;

	Ref<GFX::GfxDescriptorSetLayout> m_Layout;
	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_Buffers;
	std::array<Ref<GFX::GfxDescriptorSet>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_Sets;
};

} // namespace Aquila::Rendering
