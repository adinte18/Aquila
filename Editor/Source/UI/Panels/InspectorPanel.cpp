#include "UI/Panels/InspectorPanel.h"
#include "UI/Inspectors/CameraComponentUI.h"
#include "UI/Inspectors/LightComponentUI.h"
#include "UI/Inspectors/MaterialComponentUI.h"
#include "UI/Inspectors/TransformComponentUI.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/TextInput.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;

InspectorPanel::InspectorPanel(GFX::GfxContext &context) : m_Context(context) {}

void InspectorPanel::SetVisible(UI::Core::View *v, bool visible) {
	v->SetHidden(!visible);
}

void InspectorPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *) {
	m_ScrollView = panel->FindById("inspector-scroll");
	if (m_ScrollView == nullptr) {
		AQUILA_LOG_ERROR("InspectorPanel: 'inspector-scroll' not found in layout");
		return;
	}

	m_NameInput = panel->FindById<UI::Core::TextInput>("inspector-name");
	if (m_NameInput != nullptr) {
		SetVisible(m_NameInput, false);
	}

	auto AddUIComponent = [&](std::string_view sectionId, Unique<IComponentUI> componentUI) {
		auto *section = panel->FindById<UI::Core::Collapsible>(sectionId);
		if (section == nullptr) {
			AQUILA_LOG_ERROR("InspectorPanel: section '{}' not found in layout", sectionId);
			return;
		}
		auto *grid = section->AddContent<UI::Core::PropertyGrid>();
		componentUI->Build(section, grid);
		SetVisible(section, false);
		m_Sections.push_back({ section, std::move(componentUI) });
	};

	AddUIComponent("section-transform", CreateUnique<TransformComponentUI>());
	AddUIComponent("section-material", CreateUnique<MaterialComponentUI>(m_Context));
	AddUIComponent("section-light", CreateUnique<LightComponentUI>(m_Context));
	AddUIComponent("section-camera", CreateUnique<CameraComponentUI>());
}

void InspectorPanel::ShowEntity(Entity entity) {
	if (!m_ScrollView) {
		return;
	}

	SetVisible(m_NameInput, true);
	m_NameInput->SetText(entity.GetName());

	for (auto &section : m_Sections) {
		bool has = section.ui->Matches(entity);
		SetVisible(section.collapsible, has);
		if (has) {
			section.ui->Show(entity);
		}
	}
}

} // namespace Editor
