#pragma once

#include "UI/Panels/IEditorPanel.h"
#include "Aquila/Scene/Entity.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::UI::Core {
class View;
}

namespace Editor {

class InspectorPanel : public IEditorPanel {
  public:
	explicit InspectorPanel(Aquila::GFX::GfxContext &context);
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
	void ShowEntity(Aquila::SceneManagement::Entity entity);

  private:
	void AddTransformSection(Aquila::SceneManagement::Entity entity);
	void AddMaterialSection(Aquila::SceneManagement::Entity entity);
	void AddLightSection(Aquila::SceneManagement::Entity entity);

	Aquila::GFX::GfxContext &m_Context;
	Aquila::UI::Core::View *m_ScrollView = nullptr;
};

} // namespace Editor
