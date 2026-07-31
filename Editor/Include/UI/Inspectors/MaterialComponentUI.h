#pragma once

#include "UI/Inspectors/IComponentUI.h"

namespace Aquila::GFX {
class GfxContext;
}
namespace Aquila::UI::Core {
class ColorPicker;
class DragFloat;
class Dropdown;
class PropertyGrid;
}

namespace Editor {

class MaterialComponentUI : public IComponentUI {
  public:
	explicit MaterialComponentUI(Aquila::GFX::GfxContext &context);
	bool matches(Aquila::SceneManagement::Entity entity) const override;
	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;
	std::vector<ComponentSignal> signals(Aquila::SceneManagement::Entity entity) const override;

  private:
	Aquila::GFX::GfxContext &m_context;
	Aquila::UI::Core::Dropdown *m_type = nullptr;
	Aquila::UI::Core::ColorPicker *m_albedo = nullptr;
	Aquila::UI::Core::DragFloat *m_metallic = nullptr;
	Aquila::UI::Core::DragFloat *m_roughness = nullptr;
	Aquila::UI::Core::PropertyGrid *m_texture_area = nullptr;
};

} // namespace Editor
