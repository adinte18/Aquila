#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "UI/Panels/IEditorPanel.h"
#include "Aquila/Scene/Entity.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::UI::Core {
class View;
class TextInput;
class PropertyGrid;
class Vec3Field;
class DragFloat;
class ColorPicker;
class Toggle;
} // namespace Aquila::UI::Core

namespace Editor {

class InspectorPanel : public IEditorPanel {
  public:
	explicit InspectorPanel(Aquila::GFX::GfxContext &context);
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
	void ShowEntity(Aquila::SceneManagement::Entity entity);
	void SetVisible(Aquila::UI::Core::View *v, bool visible) {
		Aquila::UI::StyleProperties sp;
		sp.display = visible ? Aquila::UI::Display::Flex : Aquila::UI::Display::None;
		v->MergeStyle(sp);
	}

  private:
	template <typename Component, typename Fn>
	void ShowSection(Aquila::SceneManagement::Entity &entity, Aquila::UI::Core::Collapsible *section, Fn &&fn) {
		if (entity.HasComponent<Component>()) {
			SetVisible(section, true);
			fn(entity.GetComponent<Component>());
		} else {
			SetVisible(section, false);
		}
	}

	std::pair<Aquila::UI::Core::Collapsible*, Aquila::UI::Core::PropertyGrid*> BuildSection(const std::string &title);

	Aquila::GFX::GfxContext &m_Context;
	Aquila::UI::Core::View *m_ScrollView = nullptr;

	Aquila::UI::Core::TextInput *m_NameInput = nullptr;

	Aquila::UI::Core::Collapsible *m_TransformSection = nullptr;
	Aquila::UI::Core::Vec3Field *m_PositionField = nullptr;
	Aquila::UI::Core::Vec3Field *m_ScaleField = nullptr;

	Aquila::UI::Core::Collapsible *m_MaterialSection = nullptr;
	Aquila::UI::Core::ColorPicker *m_AlbedoField = nullptr;
	Aquila::UI::Core::DragFloat *m_MetallicField = nullptr;
	Aquila::UI::Core::DragFloat *m_RoughnessField = nullptr;

	Aquila::UI::Core::Collapsible *m_LightSection = nullptr;
	Aquila::UI::Core::ColorPicker *m_LightColorField = nullptr;
	Aquila::UI::Core::DragFloat *m_IntensityField = nullptr;
	Aquila::UI::Core::DragFloat *m_RangeField = nullptr;
	Aquila::UI::Core::Toggle *m_LightActiveField = nullptr;
};

} // namespace Editor
