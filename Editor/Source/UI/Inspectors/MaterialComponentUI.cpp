#include "UI/Inspectors/MaterialComponentUI.h"

#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/UI/Widgets/AssetSlot.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "UI/Inspectors/ComponentBinder.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

namespace {

std::string MaterialTypeToString(Graphics::MaterialType t) {
	switch (t) {
	case Graphics::MaterialType::PBR:
		return "PBR";
	case Graphics::MaterialType::Lit:
		return "Lit";
	case Graphics::MaterialType::Unlit:
		return "Unlit";
	case Graphics::MaterialType::Custom:
		return "Custom";
	default:
		return "Lit";
	}
}

Graphics::MaterialType StringToMaterialType(const std::string &s) {
	if (s == "PBR") {
		return Graphics::MaterialType::PBR;
	}
	if (s == "Unlit") {
		return Graphics::MaterialType::Unlit;
	}
	if (s == "Custom") {
		return Graphics::MaterialType::Custom;
	}
	return Graphics::MaterialType::Lit;
}

} // namespace

MaterialComponentUI::MaterialComponentUI(GFX::GfxContext &context) : m_Context(context) {}

bool MaterialComponentUI::Matches(Entity entity) const {
	return entity.HasComponent<MaterialComponent>();
}

void MaterialComponentUI::Build(UI::Core::Collapsible *section, UI::Core::PropertyGrid *grid) {
	m_Type = grid->AddRow<UI::Core::Dropdown>("Type");
	m_Type->AddOption("PBR");
	m_Type->AddOption("Lit");
	m_Type->AddOption("Unlit");
	m_Type->AddOption("Custom");
	m_Type->SetValue("Lit");

	m_Albedo = grid->AddRow<UI::Core::ColorPicker>("Albedo", m_Context, vec4(1.f));

	using UI::Core::DragFloat;
	m_Metallic = grid->AddRow<DragFloat>("Metallic", DragFloat::Config{ .min = 0.f, .max = 1.f, .speed = 0.01f, .precision = 3 });
	m_Roughness = grid->AddRow<DragFloat>("Roughness", DragFloat::Config{ .min = 0.f, .max = 1.f, .speed = 0.01f, .precision = 3 });

	m_TextureArea = section->AddContent<UI::Core::PropertyGrid>();
}

void MaterialComponentUI::Show(Entity entity) {
	auto &mat = entity.GetComponent<MaterialComponent>();

	ComponentBinder<MaterialComponent> bind(entity);

	m_Type->SetValue(MaterialTypeToString(mat.type));
	m_Type->onChanged.Set([entity](const std::string &v) mutable {
		entity.GetComponent<MaterialComponent>().type = StringToMaterialType(v);
	});

	bind.Bind(m_Albedo, [](auto &m) -> vec4 & { return m.surfaceProperties.albedo; });
	bind.Bind(m_Metallic, [](auto &m) -> float & { return m.surfaceProperties.metallic; });
	bind.Bind(m_Roughness, [](auto &m) -> float & { return m.surfaceProperties.roughness; });

	while (!m_TextureArea->GetChildren().empty()) {
		m_TextureArea->RemoveChild(m_TextureArea->GetChildren().front().get());
	}
	if (mat.material) {
		for (const auto &param : mat.material->GetParameters()) {
			if (param.type != Graphics::ParameterType::Texture2D) {
				continue;
			}
			m_TextureArea->AddRow<UI::Core::AssetSlot>(param.name, "Texture2D");
		}
	}
}

} // namespace Editor
