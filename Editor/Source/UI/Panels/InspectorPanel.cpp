#include "UI/Panels/InspectorPanel.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/Toggle.h"
#include "Aquila/UI/Widgets/VecField.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

InspectorPanel::InspectorPanel(GFX::GfxContext &context) : m_Context(context) {}

void InspectorPanel::Build(UI::Core::DockPanel *panel, UI::Core::View *) {
	auto scrollUniq = CreateUnique<UI::Core::View>();
	scrollUniq->SetId("inspector-scroll");

	UI::StyleProperties sp;
	sp.flexDirection = UI::FlexDirection::Column;
	sp.width = UI::StyleLength::Grow();
	sp.padding = UI::StyleEdges::All(UI::StyleLength::Pixel(8.f));
	sp.gap = 6.f;
	scrollUniq->SetStyle(sp);

	m_ScrollView = static_cast<UI::Core::View *>(panel->AddChild(std::move(scrollUniq)));
}

void InspectorPanel::ShowEntity(Entity entity) {
	if (!m_ScrollView) {
		return;
	}

	std::vector<UI::Core::View *> toRemove;
	for (auto &child : m_ScrollView->GetChildren()) {
		toRemove.push_back(child.get());
	}
	for (auto *child : toRemove) {
		m_ScrollView->RemoveChild(child);
	}

	auto nameLabel = CreateUnique<UI::Core::Label>(entity.GetName());
	nameLabel->AddClass("inspector-entity-name");
	m_ScrollView->AddChild(std::move(nameLabel));

	if (entity.HasComponent<TransformComponent>()) {
		AddTransformSection(entity);
	}
	if (entity.HasComponent<MaterialComponent>()) {
		AddMaterialSection(entity);
	}
	if (entity.HasComponent<LightComponent>()) {
		AddLightSection(entity);
	}
}

void InspectorPanel::AddTransformSection(Entity entity) {
	auto &tf = entity.GetComponent<TransformComponent>();

	auto colUniq = CreateUnique<UI::Core::Collapsible>("Transform");
	auto *col = static_cast<UI::Core::Collapsible *>(m_ScrollView->AddChild(std::move(colUniq)));

	auto gridUniq = CreateUnique<UI::Core::PropertyGrid>();
	auto *grid = static_cast<UI::Core::PropertyGrid *>(col->AddContent(std::move(gridUniq)));

	auto pos = CreateUnique<UI::Core::Vec3Field>();
	pos->SetValue(tf.GetLocalPosition());
	pos->SetSpeed(0.1f);
	pos->SetOnChanged([entity](vec3 v) mutable { entity.GetComponent<TransformComponent>().SetLocalPosition(v); });
	grid->AddRow("Position", std::move(pos));

	auto scl = CreateUnique<UI::Core::Vec3Field>();
	scl->SetValue(tf.GetLocalScale());
	scl->SetSpeed(0.1f);
	scl->SetOnChanged([entity](vec3 v) mutable { entity.GetComponent<TransformComponent>().SetLocalScale(v); });
	grid->AddRow("Scale", std::move(scl));
}

void InspectorPanel::AddMaterialSection(Entity entity) {
	auto &mat = entity.GetComponent<MaterialComponent>();

	auto colUniq = CreateUnique<UI::Core::Collapsible>("Material");
	auto *col = static_cast<UI::Core::Collapsible *>(m_ScrollView->AddChild(std::move(colUniq)));

	auto gridUniq = CreateUnique<UI::Core::PropertyGrid>();
	auto *grid = static_cast<UI::Core::PropertyGrid *>(col->AddContent(std::move(gridUniq)));

	auto albedo = CreateUnique<UI::Core::ColorPicker>(m_Context, mat.surfaceProperties.albedo);
	albedo->SetOnChanged(
		[entity](vec4 c) mutable { entity.GetComponent<MaterialComponent>().surfaceProperties.albedo = c; });
	grid->AddRow("Albedo", std::move(albedo));

	auto metallic = CreateUnique<UI::Core::DragFloat>();
	metallic->SetRange(0.f, 1.f);
	metallic->SetSpeed(0.01f);
	metallic->SetPrecision(3);
	metallic->SetValue(mat.surfaceProperties.metallic);
	metallic->SetOnChanged(
		[entity](float v) mutable { entity.GetComponent<MaterialComponent>().surfaceProperties.metallic = v; });
	grid->AddRow("Metallic", std::move(metallic));

	auto roughness = CreateUnique<UI::Core::DragFloat>();
	roughness->SetRange(0.f, 1.f);
	roughness->SetSpeed(0.01f);
	roughness->SetPrecision(3);
	roughness->SetValue(mat.surfaceProperties.roughness);
	roughness->SetOnChanged(
		[entity](float v) mutable { entity.GetComponent<MaterialComponent>().surfaceProperties.roughness = v; });
	grid->AddRow("Roughness", std::move(roughness));
}

void InspectorPanel::AddLightSection(Entity entity) {
	auto &light = entity.GetComponent<LightComponent>();

	auto colUniq = CreateUnique<UI::Core::Collapsible>("Light");
	auto *col = static_cast<UI::Core::Collapsible *>(m_ScrollView->AddChild(std::move(colUniq)));

	auto gridUniq = CreateUnique<UI::Core::PropertyGrid>();
	auto *grid = static_cast<UI::Core::PropertyGrid *>(col->AddContent(std::move(gridUniq)));

	auto color = CreateUnique<UI::Core::ColorPicker>(m_Context, vec4(light.GetColor(), 1.f));
	color->SetOnChanged([entity](vec4 c) mutable { entity.GetComponent<LightComponent>().SetColor(vec3(c)); });
	grid->AddRow("Color", std::move(color));

	auto intensity = CreateUnique<UI::Core::DragFloat>();
	intensity->SetValue(light.GetIntensity());
	intensity->SetRange(0.f, 100.f);
	intensity->SetSpeed(0.5f);
	intensity->SetOnChanged([entity](float v) mutable { entity.GetComponent<LightComponent>().SetIntensity(v); });
	grid->AddRow("Intensity", std::move(intensity));

	if (light.GetType() == LightComponent::Type::Point) {
		auto range = CreateUnique<UI::Core::DragFloat>();
		range->SetValue(light.GetRange());
		range->SetRange(0.f, 200.f);
		range->SetSpeed(0.5f);
		range->SetOnChanged([entity](float v) mutable { entity.GetComponent<LightComponent>().SetRange(v); });
		grid->AddRow("Range", std::move(range));
	}

	auto active = CreateUnique<UI::Core::Toggle>(light.IsActive());
	active->SetOnChanged([entity](bool on) mutable { entity.GetComponent<LightComponent>().SetActive(on); });
	grid->AddRow("Active", std::move(active));

	grid->AddRow("Shadows", CreateUnique<UI::Core::Checkbox>(false));
}

} // namespace Editor
