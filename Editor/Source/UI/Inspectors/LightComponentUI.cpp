#include "UI/Inspectors/LightComponentUI.h"

#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/Toggle.h"
#include "UI/Inspectors/ComponentBinder.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

LightComponentUI::LightComponentUI(GFX::GfxContext &context) : m_Context(context) {}

bool LightComponentUI::Matches(Entity entity) const {
	return entity.HasComponent<LightComponent>();
}

void LightComponentUI::Build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	using UI::Core::DragFloat;
	m_Color = grid->AddRow<UI::Core::ColorPicker>("Color", m_Context, vec4(1.f));
	m_Intensity = grid->AddRow<DragFloat>("Intensity", DragFloat::Config{ .min = 0.f, .max = 100.f, .speed = 0.5f });
	m_Range = grid->AddRow<DragFloat>("Range", DragFloat::Config{ .min = 0.f, .max = 200.f, .speed = 0.5f });
	m_Active = grid->AddRow<UI::Core::Toggle>("Active", false);
	grid->AddRow<UI::Core::Checkbox>("Shadows", false);
}

void LightComponentUI::Show(Entity entity) {
	auto &light = entity.GetComponent<LightComponent>();
	ComponentBinder<LightComponent> bind(entity);

	m_Color->SetValue(vec4(light.GetColor(), 1.f));
	m_Color->onChanged.Set([entity](vec4 c) mutable { entity.GetComponent<LightComponent>().SetColor(vec3(c)); });

	bind.Bind(m_Intensity, &LightComponent::GetIntensity, &LightComponent::SetIntensity);
	bind.Bind(m_Active, &LightComponent::IsActive, &LightComponent::SetActive);

	const bool isPoint = light.GetType() == LightComponent::Type::Point;
	UI::StyleProperties rangeSp;
	rangeSp.display = isPoint ? UI::Display::Flex : UI::Display::None;
	m_Range->GetParent()->MergeStyle(rangeSp);
	if (isPoint) {
		bind.Bind(m_Range, &LightComponent::GetRange, &LightComponent::SetRange);
	}
}

} // namespace Editor
