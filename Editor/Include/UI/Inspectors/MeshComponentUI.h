#pragma once

#include "UI/Inspectors/ReflectedComponentUI.h"

namespace Aquila::UI::Core {
class Dropdown;
class Label;
} // namespace Aquila::UI::Core

namespace Editor {

class MeshComponentUI : public ReflectedComponentUI {
  public:
	explicit MeshComponentUI(const Aquila::SceneManagement::ComponentDescriptor &descriptor);

	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;

  private:
	void update_stats(Aquila::SceneManagement::Entity entity);

	Aquila::UI::Core::Dropdown *m_primitive = nullptr;
	Aquila::UI::Core::Label *m_vertices = nullptr;
	Aquila::UI::Core::Label *m_triangles = nullptr;
};

} // namespace Editor
