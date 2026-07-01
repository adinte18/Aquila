#include "UI/Panels/InspectorPanel.h"
#include "UI/Inspectors/CameraComponentUI.h"
#include "UI/Inspectors/LightComponentUI.h"
#include "UI/Inspectors/MaterialComponentUI.h"
#include "UI/Inspectors/TransformComponentUI.h"

#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/TextInput.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;

InspectorPanel::InspectorPanel(GFX::GfxContext &context) : m_Context(context) {}

std::pair<UI::Core::Collapsible *, UI::Core::PropertyGrid *> InspectorPanel::BuildSection(const std::string &title) {
	auto *section = m_ScrollView->AddChild<UI::Core::Collapsible>(title);
	auto *grid = section->AddContent<UI::Core::PropertyGrid>();
	SetVisible(section, false);
	return { section, grid };
}

void InspectorPanel::SetVisible(UI::Core::View *v, bool visible) {
	UI::StyleProperties sp;
	sp.display = visible ? UI::Display::Flex : UI::Display::None;
	v->MergeStyle(sp);
}

void InspectorPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *) {
	m_ScrollView = panel->AddChild<UI::Core::View>();
	m_ScrollView->SetId("inspector-scroll");

	m_NameInput = m_ScrollView->AddChild<UI::Core::TextInput>();
	m_NameInput->AddClass("inspector-entity-name");
	SetVisible(m_NameInput, false);

	auto AddUIComponent = [&](const std::string &title, Unique<IComponentUI> componentUI) {
		auto [section, grid] = BuildSection(title);
		componentUI->Build(section, grid);
		m_Sections.push_back({ section, std::move(componentUI) });
	};

	AddUIComponent("Transform", CreateUnique<TransformComponentUI>());
	AddUIComponent("Material", CreateUnique<MaterialComponentUI>(m_Context));
	AddUIComponent("Light", CreateUnique<LightComponentUI>(m_Context));
	AddUIComponent("Camera", CreateUnique<CameraComponentUI>());
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
