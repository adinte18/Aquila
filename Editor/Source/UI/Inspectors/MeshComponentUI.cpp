#include "UI/Inspectors/MeshComponentUI.h"

#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/UI/Widgets/Checkbox.h"
#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"
#include "UI/Inspectors/ComponentBinder.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::SceneManagement;
using namespace Aquila::SceneManagement::Components;

namespace {

Ref<Graphics::Resources::Mesh> make_primitive(const std::string &shape) {
	using Graphics::Resources::Mesh;
	auto mesh = std::make_shared<Mesh>(shape);
	if (shape == "sphere") {
		mesh->load_from_data(Mesh::generate_sphere(0.5F, 32, 16));
	} else if (shape == "plane") {
		mesh->load_from_data(Mesh::generate_plane(1.0F, 1.0F, 1, 1));
	} else if (shape == "cylinder") {
		mesh->load_from_data(Mesh::generate_cylinder(0.5F, 1.0F, 32));
	} else {
		mesh->load_from_data(Mesh::generate_cube(1.0F));
	}
	return mesh;
}

std::string procedural_shape(const std::string &path) {
	constexpr std::string_view prefix = "procedural://";
	std::string_view view = path;
	if (!view.starts_with(prefix)) {
		return "";
	}
	return std::string(view.substr(prefix.size()));
}

} // namespace

bool MeshComponentUI::matches(Entity entity) const {
	return entity.has_component<MeshComponent>();
}

void MeshComponentUI::build(UI::Core::Collapsible *, UI::Core::PropertyGrid *grid) {
	m_primitive = grid->add_row<UI::Core::Dropdown>("Primitive");
	m_primitive->add_option("cube", "Cube");
	m_primitive->add_option("sphere", "Sphere");
	m_primitive->add_option("plane", "Plane");
	m_primitive->add_option("cylinder", "Cylinder");

	m_vertices = grid->add_row<UI::Core::Label>("Vertices", std::string("0"));
	m_triangles = grid->add_row<UI::Core::Label>("Triangles", std::string("0"));
	m_cast_shadows = grid->add_row<UI::Core::Checkbox>("Cast Shadows", false);
	m_receive_shadows = grid->add_row<UI::Core::Checkbox>("Receive Shadows", false);
}

void MeshComponentUI::update_stats(Entity entity) {
	auto &mesh = entity.get_component<MeshComponent>();
	if (mesh.is_valid()) {
		m_vertices->set_text(std::to_string(mesh.data->get_vertex_count()));
		m_triangles->set_text(std::to_string(mesh.data->get_index_count() / 3));
	} else {
		m_vertices->set_text(std::to_string(0));
		m_triangles->set_text(std::to_string(0));
	}
}

void MeshComponentUI::show(Entity entity) {
	update_stats(entity);

	auto &mesh = entity.get_component<MeshComponent>();
	m_primitive->clear_selection();
	if (mesh.is_valid()) {
		m_primitive->set_value(procedural_shape(mesh.data->get_path()));
	}
	if (m_primitive->get_value().empty()) {
		m_primitive->set_placeholder(mesh.is_valid() ? "Custom" : "Select…");
	}

	m_primitive->on_changed.set([this, entity](const std::string &shape) mutable {
		entity.get_component<MeshComponent>().set_mesh(make_primitive(shape));
		update_stats(entity);
	});

	ComponentBinder<MeshComponent> bind(entity);
	bind.bind(m_cast_shadows, [](auto &c) -> bool & { return c.cast_shadows; })
		.on_change([&mesh](const bool &val) { mesh.cast_shadows = val; });
	bind.bind(m_receive_shadows, [](auto &c) -> bool & { return c.receive_shadows; })
		.on_change([&mesh](const bool& val) { mesh.receive_shadows = val; });
}

} // namespace Editor
