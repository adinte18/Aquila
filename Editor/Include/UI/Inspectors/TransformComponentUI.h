#pragma once

#include "UI/Inspectors/IComponentUI.h"

namespace Aquila::UI::Core {
class Vec3Field;
}

namespace Editor {

class TransformComponentUI : public IComponentUI {
  public:
	bool matches(Aquila::SceneManagement::Entity entity) const override;
	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;
	std::vector<ComponentSignal> signals(Aquila::SceneManagement::Entity entity) const override;

  private:
	Aquila::UI::Core::Vec3Field *m_position = nullptr;
	Aquila::UI::Core::Vec3Field *m_scale = nullptr;
	Aquila::UI::Core::Vec3Field *m_rotation = nullptr;
};

} // namespace Editor
