#include "UI/Inspectors/MeshComponentUI.h"

#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/PropertyGrid.h"

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

MeshComponentUI::MeshComponentUI(const ComponentDescriptor &descriptor, UI::Core::TextureCache *textures)
	: ReflectedComponentUI(descriptor, nullptr), m_textures(textures) {}

void MeshComponentUI::build(UI::Core::Collapsible *section, UI::Core::PropertyGrid *grid) {
	auto icon = [this](const char *name) -> GFX::GfxTexture * {
		return m_textures != nullptr ? m_textures->load("Engine/UI/Icons/" + std::string(name) + ".svg") : nullptr;
	};

	m_primitive = grid->add_row<UI::Core::Dropdown>("Primitive");
	m_primitive->set_icon(icon("cuboid"));
	m_primitive->add_option("cube", "Cube", icon("box"));
	m_primitive->add_option("sphere", "Sphere", icon("circle"));
	m_primitive->add_option("plane", "Plane", icon("square"));
	m_primitive->add_option("cylinder", "Cylinder", icon("cylinder"));

	m_vertices = grid->add_row<UI::Core::Label>("Vertices", std::string("0"));
	m_triangles = grid->add_row<UI::Core::Label>("Triangles", std::string("0"));

	ReflectedComponentUI::build(section, grid);
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
	ReflectedComponentUI::show(entity);
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
}

} // namespace Editor
