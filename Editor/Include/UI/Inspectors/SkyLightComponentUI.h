#pragma once

#include "UI/Inspectors/IComponentUI.h"

namespace Aquila::GFX {
class GfxContext;
}
namespace Aquila::UI::Core {
class ColorPicker;
class DragFloat;
class Dropdown;
class Toggle;
} // namespace Aquila::UI::Core

namespace Editor {

class SkyLightComponentUI : public IComponentUI {
  public:
	explicit SkyLightComponentUI(Aquila::GFX::GfxContext &context);
	bool matches(Aquila::SceneManagement::Entity entity) const override;
	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;

  private:
	Aquila::GFX::GfxContext &m_context;
	Aquila::UI::Core::Dropdown *m_source = nullptr;
	Aquila::UI::Core::DragFloat *m_sun_elevation = nullptr;
	Aquila::UI::Core::DragFloat *m_sun_azimuth = nullptr;
	Aquila::UI::Core::DragFloat *m_turbidity = nullptr;
	Aquila::UI::Core::ColorPicker *m_ground_albedo = nullptr;
	Aquila::UI::Core::DragFloat *m_intensity = nullptr;
	Aquila::UI::Core::ColorPicker *m_tint = nullptr;
	Aquila::UI::Core::Toggle *m_active = nullptr;
	Aquila::UI::Core::Toggle *m_render_skybox = nullptr;
};

} // namespace Editor
