#pragma once

#include "UI/Inspectors/IComponentUI.h"

#include "Aquila/UI/Widgets/ReflectedPropertyGrid.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::SceneManagement {
class ComponentDescriptor;
}

namespace Editor {

class ReflectedComponentUI : public IComponentUI {
  public:
	ReflectedComponentUI(const Aquila::SceneManagement::ComponentDescriptor &descriptor,
						 Aquila::GFX::GfxContext *context);

	bool matches(Aquila::SceneManagement::Entity entity) const override;
	void build(Aquila::UI::Core::Collapsible *section, Aquila::UI::Core::PropertyGrid *grid) override;
	void show(Aquila::SceneManagement::Entity entity) override;
	std::vector<ComponentSignal> signals(Aquila::SceneManagement::Entity entity) const override;

  protected:
	[[nodiscard]] const Aquila::SceneManagement::ComponentDescriptor &get_descriptor() const { return m_descriptor; }
	[[nodiscard]] Aquila::UI::Core::ReflectedPropertyGrid *get_properties() { return m_properties.get(); }

  private:
	const Aquila::SceneManagement::ComponentDescriptor &m_descriptor;
	Aquila::GFX::GfxContext *m_context;
	Unique<Aquila::UI::Core::ReflectedPropertyGrid> m_properties;
};

}
