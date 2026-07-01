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
	bool Matches(Aquila::SceneManagement::Entity entity) const override;
	void Build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void Show(Aquila::SceneManagement::Entity entity) override;

  private:
	Aquila::GFX::GfxContext &m_Context;
	Aquila::UI::Core::Dropdown *m_Type = nullptr;
	Aquila::UI::Core::ColorPicker *m_Albedo = nullptr;
	Aquila::UI::Core::DragFloat *m_Metallic = nullptr;
	Aquila::UI::Core::DragFloat *m_Roughness = nullptr;
	Aquila::UI::Core::PropertyGrid *m_TextureArea = nullptr;
};

} // namespace Editor
