#pragma once

#include "UI/Inspectors/ReflectedComponentUI.h"

namespace Aquila::UI::Core {
class PropertyGrid;
}

namespace Editor {

class MaterialComponentUI : public ReflectedComponentUI {
  public:
	MaterialComponentUI(const Aquila::SceneManagement::ComponentDescriptor &descriptor,
						Aquila::GFX::GfxContext &context);

	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;

  private:
	Aquila::UI::Core::PropertyGrid *m_texture_area = nullptr;
};

} // namespace Editor
