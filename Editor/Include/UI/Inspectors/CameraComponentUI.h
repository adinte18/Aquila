#pragma once

#include "UI/Inspectors/IComponentUI.h"

namespace Aquila::UI::Core {
class Checkbox;
class DragFloat;
class Toggle;
} // namespace Aquila::UI::Core

namespace Editor {

class CameraComponentUI : public IComponentUI {
  public:
	bool Matches(Aquila::SceneManagement::Entity entity) const override;
	void Build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void Show(Aquila::SceneManagement::Entity entity) override;

  private:
	Aquila::UI::Core::DragFloat *m_Fov = nullptr;
	Aquila::UI::Core::DragFloat *m_Near = nullptr;
	Aquila::UI::Core::DragFloat *m_Far = nullptr;
	Aquila::UI::Core::Toggle *m_Primary = nullptr;
	Aquila::UI::Core::Checkbox *m_Ortho = nullptr;
};

} // namespace Editor
