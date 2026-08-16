#pragma once

#include "UI/Inspectors/IComponentUI.h"

namespace Aquila::UI::Core {
class Checkbox;
class Dropdown;
class Label;
} // namespace Aquila::UI::Core

namespace Editor {

class MeshComponentUI : public IComponentUI {
  public:
	bool matches(Aquila::SceneManagement::Entity entity) const override;
	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;

  private:
	void update_stats(Aquila::SceneManagement::Entity entity);

	Aquila::UI::Core::Dropdown *m_primitive = nullptr;
	Aquila::UI::Core::Label *m_vertices = nullptr;
	Aquila::UI::Core::Label *m_triangles = nullptr;
	Aquila::UI::Core::Checkbox *m_cast_shadows = nullptr;
	Aquila::UI::Core::Checkbox *m_receive_shadows = nullptr;
};

} // namespace Editor
