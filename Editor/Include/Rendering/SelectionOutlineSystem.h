#pragma once

#include "Aquila/Rendering/Systems/RenderingSystemBase.h"
#include "Aquila/Graphics/Shader/ReloadablePipeline.h"
#include "Aquila/Scene/Entity.h"

namespace Editor {

class SelectionOutlineSystem : public Aquila::Rendering::RenderingSystemBase {
  public:
	SelectionOutlineSystem() = default;
	~SelectionOutlineSystem() override = default;

	void on_init(Aquila::GFX::GfxContext &ctx) override;
	void add_passes(Aquila::Graphics::RG::RenderGraph &graph, Aquila::Rendering::FrameContext &ctx) override;

	void set_selected_entity(Aquila::SceneManagement::Entity entity) { m_selected = entity; }
	void clear_selection() { m_selected = Aquila::SceneManagement::Entity::null(); }

	void set_outline_color(Vec4 color) { m_outline_color = color; }
	void set_thickness(F32 pixels) { m_thickness = pixels; }

  private:
	Aquila::SceneManagement::Entity m_selected;

	Vec4 m_outline_color{ 0.290F, 0.620F, 0.373F, 1.F };
	F32 m_thickness = 2.F;

	Ref<Aquila::Graphics::Shader::ReloadablePipeline> m_mask_pipeline;
	Ref<Aquila::Graphics::Shader::ReloadablePipeline> m_outline_pipeline;

	Ref<Aquila::GFX::GfxDescriptorSetLayout> m_outline_layout;
	std::array<Ref<Aquila::GFX::GfxDescriptorSet>, Aquila::SharedConstants::MAX_FRAMES_IN_FLIGHT> m_outline_sets;
};

} // namespace Editor
