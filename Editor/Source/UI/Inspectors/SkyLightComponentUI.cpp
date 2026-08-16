#include "UI/Inspectors/SkyLightComponentUI.h"

#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "Aquila/UI/Widgets/Toggle.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

SkyLightComponentUI::SkyLightComponentUI(GFX::GfxContext &context) : m_context(context) {}

bool SkyLightComponentUI::matches(Entity entity) const {
	return entity.has_component<SkyLightComponent>();
}

void SkyLightComponentUI::build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	using UI::Core::DragFloat;

	m_source = grid->add_row<UI::Core::Dropdown>("Source");
	m_source->add_option("Procedural");
	m_source->add_option("HDR Image");

	m_sun_elevation = grid->add_row<DragFloat>(
		"Sun Elevation", DragFloat::Config{ .min = -10.F, .max = 90.F, .speed = 0.5f, .precision = 1 });
	m_sun_azimuth = grid->add_row<DragFloat>(
		"Sun Azimuth", DragFloat::Config{ .min = 0.F, .max = 360.F, .speed = 1.F, .precision = 1 });
	m_turbidity = grid->add_row<DragFloat>(
		"Turbidity", DragFloat::Config{ .min = 1.F, .max = 10.F, .speed = 0.05f, .precision = 2 });
	m_ground_albedo = grid->add_row<UI::Core::ColorPicker>("Ground Albedo", m_context, Vec4(0.3F, 0.3F, 0.3F, 1.F));

	m_intensity = grid->add_row<DragFloat>(
		"Intensity", DragFloat::Config{ .min = 0.F, .max = 20.F, .speed = 0.05f, .precision = 3 });
	m_tint = grid->add_row<UI::Core::ColorPicker>("Tint", m_context, Vec4(1.F));

	m_active = grid->add_row<UI::Core::Toggle>("Active", false);
	m_render_skybox = grid->add_row<UI::Core::Toggle>("Render Skybox", false);
}

void SkyLightComponentUI::show(Entity entity) {
	auto &sky = entity.get_component<SkyLightComponent>();

	m_source->set_value(sky.get_source() == SkySource::Procedural ? "Procedural" : "HDR Image");
	m_source->on_changed.set([entity](const std::string &v) mutable {
		entity.get_component<SkyLightComponent>().set_source(v == "Procedural" ? SkySource::Procedural
																			   : SkySource::HdrImage);
	});

	m_sun_elevation->set_value(sky.get_sun_elevation());
	m_sun_elevation->on_changed.set(
		[entity](float v) mutable { entity.get_component<SkyLightComponent>().set_sun_elevation(v); });

	m_sun_azimuth->set_value(sky.get_sun_azimuth());
	m_sun_azimuth->on_changed.set(
		[entity](float v) mutable { entity.get_component<SkyLightComponent>().set_sun_azimuth(v); });

	m_turbidity->set_value(sky.get_turbidity());
	m_turbidity->on_changed.set(
		[entity](float v) mutable { entity.get_component<SkyLightComponent>().set_turbidity(v); });

	m_ground_albedo->set_value(Vec4(sky.get_ground_albedo(), 1.F));
	m_ground_albedo->on_changed.set(
		[entity](Vec4 c) mutable { entity.get_component<SkyLightComponent>().set_ground_albedo(Vec3(c)); });

	m_intensity->set_value(sky.get_intensity());
	m_intensity->on_changed.set(
		[entity](float v) mutable { entity.get_component<SkyLightComponent>().set_intensity(v); });

	m_tint->set_value(Vec4(sky.get_tint(), 1.F));
	m_tint->on_changed.set([entity](Vec4 c) mutable { entity.get_component<SkyLightComponent>().set_tint(Vec3(c)); });

	m_active->set_value(sky.is_active());
	m_active->on_changed.set([entity](bool v) mutable { entity.get_component<SkyLightComponent>().set_active(v); });

	m_render_skybox->set_value(sky.m_render_skybox);
	m_render_skybox->on_changed.set(
		[entity](bool v) mutable { entity.get_component<SkyLightComponent>().set_render_skybox(v); });
}

} // namespace Editor
