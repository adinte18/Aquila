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
	SceneFrameData(GFX::GfxContext &ctx, Uint32 width, Uint32 height);

	void update(SceneManagement::Scene &scene, float delta_time, Uint32 frame_slot);

	void on_resize(Uint32 width, Uint32 height);

	[[nodiscard]] GFX::GfxDescriptorSet &get_descriptor_set(Uint32 frame_slot) const;
	[[nodiscard]] GFX::GfxDescriptorSetLayout &get_layout() const { return *m_layout; }

	[[nodiscard]] GFX::GfxBuffer &get_light_index_list_buffer() const { return *m_light_index_list_buffer; }
	[[nodiscard]] GFX::GfxBuffer &get_cluster_light_info_buffer() const { return *m_cluster_light_info_buffer; }

  private:
	GFX::GfxContext &m_ctx;
	Uint32 m_width = 0;
	Uint32 m_height = 0;
	Uint32 m_frame_index = 0;
	float m_time = 0.F;

	Ref<GFX::GfxDescriptorSetLayout> m_layout;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_frame_buffers;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_light_buffers;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_env_buffers;

	std::array<Ref<GFX::GfxBuffer>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_material_buffers;

	Ref<GFX::GfxBuffer> m_light_index_list_buffer;

	Ref<GFX::GfxBuffer> m_cluster_light_info_buffer;

	std::array<Ref<GFX::GfxDescriptorSet>, SharedConstants::MAX_FRAMES_IN_FLIGHT> m_sets;
};

} // namespace Aquila::Rendering
