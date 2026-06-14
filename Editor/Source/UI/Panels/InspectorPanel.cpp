#include "UI/Panels/InspectorPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/UI/Widgets/Toggle.h"
#include "Aquila/UI/Widgets/VecField.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

InspectorPanel::InspectorPanel(GFX::GfxContext &context) : m_Context(context) {}

std::pair<UI::Core::Collapsible*, UI::Core::PropertyGrid*> InspectorPanel::BuildSection(const std::string &title) {
	auto col = CreateUnique<UI::Core::Collapsible>(title);
	auto *section = static_cast<UI::Core::Collapsible *>(m_ScrollView->AddChild(std::move(col)));
	auto grid = CreateUnique<UI::Core::PropertyGrid>();
	auto *g = static_cast<UI::Core::PropertyGrid *>(section->AddContent(std::move(grid)));
	SetVisible(section, false);
	return { section, g };
}

void InspectorPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *) {
	auto scrollUniq = CreateUnique<UI::Core::View>();
	scrollUniq->SetId("inspector-scroll");
	m_ScrollView = static_cast<UI::Core::View *>(panel->AddChild(std::move(scrollUniq)));

	auto nameInput = CreateUnique<UI::Core::TextInput>();
	nameInput->AddClass("inspector-entity-name");
	m_NameInput = static_cast<UI::Core::TextInput *>(m_ScrollView->AddChild(std::move(nameInput)));
	SetVisible(m_NameInput, false);

	{
		auto [section, grid] = BuildSection("Transform");
		m_TransformSection = section;

		auto pos = CreateUnique<UI::Core::Vec3Field>();
		pos->SetSpeed(0.1f);
		m_PositionField = static_cast<UI::Core::Vec3Field *>(grid->AddRow("Position", std::move(pos)));

		auto scl = CreateUnique<UI::Core::Vec3Field>();
		scl->SetSpeed(0.1f);
		m_ScaleField = static_cast<UI::Core::Vec3Field *>(grid->AddRow("Scale", std::move(scl)));
	}

	{
		auto [section, grid] = BuildSection("Material");
		m_MaterialSection = section;

		auto albedo = CreateUnique<UI::Core::ColorPicker>(m_Context, vec4(1.f));
		m_AlbedoField = static_cast<UI::Core::ColorPicker *>(grid->AddRow("Albedo", std::move(albedo)));

		auto metallic = CreateUnique<UI::Core::DragFloat>();
		metallic->SetRange(0.f, 1.f);
		metallic->SetSpeed(0.01f);
		metallic->SetPrecision(3);
		m_MetallicField = static_cast<UI::Core::DragFloat *>(grid->AddRow("Metallic", std::move(metallic)));

		auto roughness = CreateUnique<UI::Core::DragFloat>();
		roughness->SetRange(0.f, 1.f);
		roughness->SetSpeed(0.01f);
		roughness->SetPrecision(3);
		m_RoughnessField = static_cast<UI::Core::DragFloat *>(grid->AddRow("Roughness", std::move(roughness)));
	}

	{
		auto [section, grid] = BuildSection("Light");
		m_LightSection = section;

		auto color = CreateUnique<UI::Core::ColorPicker>(m_Context, vec4(1.f));
		m_LightColorField = static_cast<UI::Core::ColorPicker *>(grid->AddRow("Color", std::move(color)));

		auto intensity = CreateUnique<UI::Core::DragFloat>();
		intensity->SetRange(0.f, 100.f);
		intensity->SetSpeed(0.5f);
		m_IntensityField = static_cast<UI::Core::DragFloat *>(grid->AddRow("Intensity", std::move(intensity)));

		auto range = CreateUnique<UI::Core::DragFloat>();
		range->SetRange(0.f, 200.f);
		range->SetSpeed(0.5f);
		m_RangeField = static_cast<UI::Core::DragFloat *>(grid->AddRow("Range", std::move(range)));

		auto active = CreateUnique<UI::Core::Toggle>(false);
		m_LightActiveField = static_cast<UI::Core::Toggle *>(grid->AddRow("Active", std::move(active)));

		grid->AddRow("Shadows", CreateUnique<UI::Core::Checkbox>(false));
	}
}

void InspectorPanel::ShowEntity(Entity entity) {
	if (!m_ScrollView) {
		return;
	}

	SetVisible(m_NameInput, true);
	m_NameInput->SetText(entity.GetName());

	auto Bind = [](auto *field, auto value, auto setter) {
		field->SetValue(value);
		field->SetOnChanged(std::move(setter));
	};

	ShowSection<TransformComponent>(entity, m_TransformSection, [&](auto &tf) {
		Bind(m_PositionField, tf.GetLocalPosition(),
			 [entity](vec3 v) mutable { entity.GetComponent<TransformComponent>().SetLocalPosition(v); });
		Bind(m_ScaleField, tf.GetLocalScale(),
			 [entity](vec3 v) mutable { entity.GetComponent<TransformComponent>().SetLocalScale(v); });
	});

	ShowSection<MaterialComponent>(entity, m_MaterialSection, [&](auto &mat) {
		m_AlbedoField->SetColor(mat.surfaceProperties.albedo);
		m_AlbedoField->SetOnChanged(
			[entity](vec4 c) mutable { entity.GetComponent<MaterialComponent>().surfaceProperties.albedo = c; });
		Bind(m_MetallicField, mat.surfaceProperties.metallic,
			 [entity](float v) mutable { entity.GetComponent<MaterialComponent>().surfaceProperties.metallic = v; });
		Bind(m_RoughnessField, mat.surfaceProperties.roughness,
			 [entity](float v) mutable { entity.GetComponent<MaterialComponent>().surfaceProperties.roughness = v; });
	});

	ShowSection<LightComponent>(entity, m_LightSection, [&](auto &light) {
		m_LightColorField->SetColor(vec4(light.GetColor(), 1.f));
		m_LightColorField->SetOnChanged(
			[entity](vec4 c) mutable { entity.GetComponent<LightComponent>().SetColor(vec3(c)); });
		Bind(m_IntensityField, light.GetIntensity(),
			 [entity](float v) mutable { entity.GetComponent<LightComponent>().SetIntensity(v); });

		const bool isPoint = light.GetType() == LightComponent::Type::Point;
		SetVisible(m_RangeField->GetParent(), isPoint);
		if (isPoint) {
			Bind(m_RangeField, light.GetRange(),
				 [entity](float v) mutable { entity.GetComponent<LightComponent>().SetRange(v); });
		}

		m_LightActiveField->SetOn(light.IsActive());
		m_LightActiveField->SetOnChanged(
			[entity](bool on) mutable { entity.GetComponent<LightComponent>().SetActive(on); });
	});
}

} // namespace Editor
