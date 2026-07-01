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
	bool Matches(Aquila::SceneManagement::Entity entity) const override;
	void Build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void Show(Aquila::SceneManagement::Entity entity) override;

  private:
	Aquila::GFX::GfxContext &m_Context;
	Aquila::UI::Core::ColorPicker *m_Color = nullptr;
	Aquila::UI::Core::DragFloat *m_Intensity = nullptr;
	Aquila::UI::Core::DragFloat *m_Range = nullptr;
	Aquila::UI::Core::Toggle *m_Active = nullptr;
};

} // namespace Editor
