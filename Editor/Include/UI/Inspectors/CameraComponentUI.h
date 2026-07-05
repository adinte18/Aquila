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
	bool matches(Aquila::SceneManagement::Entity entity) const override;
	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;

  private:
	Aquila::UI::Core::DragFloat *m_fov = nullptr;
	Aquila::UI::Core::DragFloat *m_near = nullptr;
	Aquila::UI::Core::DragFloat *m_far = nullptr;
	Aquila::UI::Core::Toggle *m_primary = nullptr;
	Aquila::UI::Core::Checkbox *m_ortho = nullptr;
};

} // namespace Editor
