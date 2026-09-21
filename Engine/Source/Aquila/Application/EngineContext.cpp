#include "Aquila/Application/EngineContext.h"

#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/Rendering/Systems/ObjectPickingSystem.h"

namespace Aquila::Application {

Window &EngineContext::get_window() const {
	return m_application.get_window();
}

GFX::GfxContext &EngineContext::get_context() const {
	return m_application.get_context();
}

SceneManagement::Scene &EngineContext::get_scene() const {
	return m_application.get_scene();
}

Rendering::RenderPipeline &EngineContext::get_render_pipeline() const {
	return m_application.get_render_pipeline();
}

Rendering::Renderer &EngineContext::get_renderer() const {
	return m_application.get_renderer();
}

Rendering::Renderer2D &EngineContext::get_renderer_2d() const {
	return m_application.get_renderer2_d();
}

Rendering::ObjectPickingSystem &EngineContext::get_object_picking() const {
	return m_application.get_object_picking();
}

GFX::GfxTexture &EngineContext::get_render_output() const {
	return m_application.get_render_output();
}

Rendering::IRenderWindowHost &EngineContext::get_window_host() const {
	return m_application;
}

void EngineContext::request_render_resize(Uint32 width, Uint32 height) const {
	m_application.request_render_resize(width, height);
}

void EngineContext::request_close() const {
	m_application.close();
}

}
