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

std::string material_type_to_string(Graphics::MaterialType t) {
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

Graphics::MaterialType string_to_material_type(const std::string &s) {
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

MaterialComponentUI::MaterialComponentUI(GFX::GfxContext &context) : m_context(context) {}

bool MaterialComponentUI::matches(Entity entity) const {
	return entity.has_component<MaterialComponent>();
}

void MaterialComponentUI::build(UI::Core::Collapsible *section, UI::Core::PropertyGrid *grid) {
	m_type = grid->add_row<UI::Core::Dropdown>("Type");
	m_type->add_option("PBR");
	m_type->add_option("Lit");
	m_type->add_option("Unlit");
	m_type->add_option("Custom");
	m_type->set_value("Lit");

	m_albedo = grid->add_row<UI::Core::ColorPicker>("Albedo", m_context, Vec4(1.F));

	using UI::Core::DragFloat;
	m_metallic = grid->add_row<DragFloat>("Metallic",
										  DragFloat::Config{ .min = 0.F, .max = 1.F, .speed = 0.01F, .precision = 3 });
	m_roughness = grid->add_row<DragFloat>("Roughness",
										   DragFloat::Config{ .min = 0.F, .max = 1.F, .speed = 0.01F, .precision = 3 });

	m_texture_area = section->add_child<UI::Core::PropertyGrid>();
}

void MaterialComponentUI::show(Entity entity) {
	auto &mat = entity.get_component<MaterialComponent>();

	ComponentBinder<MaterialComponent> bind(entity, &MaterialComponent::on_changed);

	m_type->set_value(material_type_to_string(mat.type));
	m_type->on_changed.set([entity](const std::string &v) mutable {
		auto &component = entity.get_component<MaterialComponent>();
		component.type = string_to_material_type(v);
		component.on_changed();
	});

	bind.bind(m_albedo, [](auto &m) -> Vec4 & { return m.surface_properties.albedo; });
	bind.bind(m_metallic, [](auto &m) -> float & { return m.surface_properties.metallic; });
	bind.bind(m_roughness, [](auto &m) -> float & { return m.surface_properties.roughness; });

	while (!m_texture_area->get_children().empty()) {
		m_texture_area->remove_child(m_texture_area->get_children().front().get());
	}
	if (mat.material) {
		for (const auto &param : mat.material->get_parameters()) {
			if (param.type != Graphics::ParameterType::Texture2D) {
				continue;
			}
			m_texture_area->add_row<UI::Core::AssetSlot>(param.name, "Texture2D");
		}
	}
}

std::vector<ComponentSignal> MaterialComponentUI::signals(Entity entity) const {
	return { { "changed", &entity.get_component<MaterialComponent>().on_changed } };
}

} // namespace Editor
