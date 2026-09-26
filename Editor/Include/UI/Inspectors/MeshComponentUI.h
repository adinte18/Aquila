#pragma once

#include "UI/Inspectors/ReflectedComponentUI.h"

namespace Aquila::UI::Core {
class Dropdown;
class Label;
class TextureCache;
} // namespace Aquila::UI::Core

namespace Editor {

class MeshComponentUI : public ReflectedComponentUI {
  public:
	MeshComponentUI(const Aquila::SceneManagement::ComponentDescriptor &descriptor,
					Aquila::UI::Core::TextureCache *textures);

	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;

  private:
	void update_stats(Aquila::SceneManagement::Entity entity);

	Aquila::UI::Core::TextureCache *m_textures = nullptr;
	Aquila::UI::Core::Dropdown *m_primitive = nullptr;
	Aquila::UI::Core::Label *m_vertices = nullptr;
	Aquila::UI::Core::Label *m_triangles = nullptr;
};

} // namespace Editor
