#include "Core/EditorContext.h"

#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Core/View.h"

namespace Editor {

using Aquila::SceneManagement::Entity;

void EditorSelection::select(Entity entity) {
	if (!entity.is_valid()) {
		clear();
		return;
	}
	if (m_entity == entity) {
		return;
	}
	m_entity = entity;
	on_changed(m_entity);
}

void EditorSelection::clear() {
	if (!m_entity.is_valid()) {
		return;
	}
	m_entity = Entity::null();
	on_changed(m_entity);
}

bool EditorSelection::has() const {
	return m_entity.is_valid() && m_entity.exists();
}

EditorContext::EditorContext(Aquila::Application::EngineContext &engine, Aquila::UI::Core::Canvas &canvas,
							 Aquila::UI::Core::LayoutLoader &layouts, Aquila::UI::Core::TextureCache &textures,
							 std::string layout_dir)
	: m_engine(engine), m_canvas(canvas), m_layouts(layouts), m_textures(textures),
	  m_layout_dir(std::move(layout_dir)) {}

Aquila::GFX::GfxTexture *EditorContext::icon(std::string_view name) const {
	return m_textures.load("Engine/UI/Icons/" + std::string(name) + ".svg");
}

Unique<Aquila::UI::Core::View> EditorContext::load_layout(std::string_view file_name) const {
	return m_layouts.load_file(m_layout_dir + std::string(file_name));
}

} // namespace Editor
