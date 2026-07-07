#include "Aquila/UI/Core/CanvasManager.h"

namespace Aquila::UI::Core {

CanvasManager::CanvasManager(Uint32 width, Uint32 height) {
	for (auto &layer : m_layers) {
		layer = std::make_unique<Canvas>(width, height);
	}
}

Canvas &CanvasManager::get_layer(UILayer layer) {
	return *m_layers[static_cast<Uint8>(layer)];
}

void CanvasManager::on_event(Application::Events::Event &e) {
	for (int i = static_cast<int>(m_layers.size()) - 1; i >= 0; i--) {
		m_layers[i]->on_event(e);
		if (e.handled) {
			break;
		}
	}
}

void CanvasManager::update(float delta_time) {
	for (auto &layer : m_layers) {
		layer->update(delta_time);
	}
}

void CanvasManager::compute() {
	for (auto &layer : m_layers) {
		layer->compute();
	}
}

void CanvasManager::render_layers(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd, UILayer from, UILayer to) {
	const int first = static_cast<int>(from);
	const int last = static_cast<int>(to);
	for (int i = first; i <= last; i++) {
		m_layers[i]->submit_to_quad_batcher(r2d, cmd);
	}
}

bool CanvasManager::is_any_layer_dirty(UILayer from, UILayer to) const {
	for (int i = static_cast<int>(from); i <= static_cast<int>(to); i++) {
		if (m_layers[i]->is_draw_list_dirty()) {
			return true;
		}
	}
	return false;
}

void CanvasManager::clear_layer_dirty_flags(UILayer from, UILayer to) {
	for (int i = static_cast<int>(from); i <= static_cast<int>(to); i++) {
		m_layers[i]->clear_draw_list_dirty();
	}
}

void CanvasManager::resize(Uint32 width, Uint32 height) {
	for (auto &layer : m_layers) {
		layer->resize(width, height);
	}
}

} // namespace Aquila::UI::Core
