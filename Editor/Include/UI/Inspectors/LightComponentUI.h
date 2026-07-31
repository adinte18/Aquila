#pragma once

#include "UI/Inspectors/IComponentUI.h"

namespace Aquila::GFX {
class GfxContext;
}
namespace Aquila::UI::Core {
class ColorPicker;
class DragFloat;
class Toggle;
} // namespace Aquila::UI::Core

namespace Editor {

class LightComponentUI : public IComponentUI {
  public:
	explicit LightComponentUI(Aquila::GFX::GfxContext &context);
	bool matches(Aquila::SceneManagement::Entity entity) const override;
	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;
	std::vector<ComponentSignal> signals(Aquila::SceneManagement::Entity entity) const override;

  private:
	Aquila::GFX::GfxContext &m_context;
	Aquila::UI::Core::ColorPicker *m_color = nullptr;
	Aquila::UI::Core::DragFloat *m_intensity = nullptr;
	Aquila::UI::Core::DragFloat *m_range = nullptr;
	Aquila::UI::Core::Toggle *m_active = nullptr;
};

} // namespace Editor
