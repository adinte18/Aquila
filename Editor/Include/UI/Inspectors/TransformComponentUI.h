#pragma once

#include "UI/Inspectors/IComponentUI.h"

namespace Aquila::UI::Core {
class Vec3Field;
}

namespace Editor {

class TransformComponentUI : public IComponentUI {
  public:
	bool Matches(Aquila::SceneManagement::Entity entity) const override;
	void Build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void Show(Aquila::SceneManagement::Entity entity) override;

  private:
	Aquila::UI::Core::Vec3Field *m_Position = nullptr;
	Aquila::UI::Core::Vec3Field *m_Scale = nullptr;
};

} // namespace Editor
