#pragma once

#include "Aquila/Scene/Entity.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "UI/Inspectors/IComponentUI.h"
#include "UI/Panels/IEditorPanel.h"

namespace Aquila::GFX {
class GfxContext;
}
namespace Aquila::UI::Core {
class Collapsible;
class DockPanel;
class PropertyGrid;
class TextInput;
class View;
} // namespace Aquila::UI::Core

namespace Editor {

class InspectorPanel : public IEditorPanel {
  public:
	explicit InspectorPanel(Aquila::GFX::GfxContext &context);
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
	void ShowEntity(Aquila::SceneManagement::Entity entity);

  private:
	std::pair<Aquila::UI::Core::Collapsible *, Aquila::UI::Core::PropertyGrid *> BuildSection(const std::string &title);
	void SetVisible(Aquila::UI::Core::View *v, bool visible);

	Aquila::GFX::GfxContext &m_Context;
	Aquila::UI::Core::View *m_ScrollView = nullptr;
	Aquila::UI::Core::TextInput *m_NameInput = nullptr;

	struct Section {
		Aquila::UI::Core::Collapsible *collapsible = nullptr;
		Unique<IComponentUI> ui;
	};
	std::vector<Section> m_Sections;
};

} // namespace Editor
