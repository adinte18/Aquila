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

LightComponentUI::LightComponentUI(GFX::GfxContext &context) : m_context(context) {}

bool LightComponentUI::matches(Entity entity) const {
	return entity.has_component<LightComponent>();
}

void LightComponentUI::build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	using UI::Core::DragFloat;
	m_color = grid->add_row<UI::Core::ColorPicker>("Color", m_context, Vec4(1.F));
	m_intensity = grid->add_row<DragFloat>("Intensity", DragFloat::Config{ .min = 0.F, .max = 100.F, .speed = 0.5F });
	m_range = grid->add_row<DragFloat>("Range", DragFloat::Config{ .min = 0.F, .max = 200.F, .speed = 0.5F });
	m_active = grid->add_row<UI::Core::Toggle>("Active", false);
	grid->add_row<UI::Core::Checkbox>("Shadows", false);
}

void LightComponentUI::show(Entity entity) {
	auto &light = entity.get_component<LightComponent>();
	ComponentBinder<LightComponent> bind(entity, &LightComponent::on_changed);

	m_color->set_value(Vec4(light.get_color(), 1.F));
	m_color->on_changed.set([entity](Vec4 c) mutable {
		auto &component = entity.get_component<LightComponent>();
		component.set_color(Vec3(c));
		component.on_changed();
	});

	bind.bind(m_intensity, &LightComponent::get_intensity, &LightComponent::set_intensity);
	bind.bind(m_active, &LightComponent::is_active, &LightComponent::set_active);

	const bool is_point = light.get_type() == LightComponent::Type::Point;
	m_range->get_parent()->set_hidden(!is_point);
	if (is_point) {
		bind.bind(m_range, &LightComponent::get_range, &LightComponent::set_range);
	}
}

std::vector<ComponentSignal> LightComponentUI::signals(Entity entity) const {
	return { { "changed", &entity.get_component<LightComponent>().on_changed } };
}

} // namespace Editor
