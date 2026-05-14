#include "Aquila/UI/Core/ViewSystem.h"

namespace Aquila::UI::Core {

ViewSystem::ViewSystem(uint32 width, uint32 height) {
	for (auto &layer : m_Layers) {
		layer = CreateUnique<Canvas>(width, height);
	}
}

Canvas &ViewSystem::GetLayer(UILayer layer) {
	return *m_Layers[static_cast<uint8>(layer)];
}

void ViewSystem::OnEvent(Application::Events::Event &e) {
	// Overlay has priority, this stops propagation if it handles the event
	for (int i = static_cast<int>(m_Layers.size()) - 1; i >= 0; i--) {
		m_Layers[i]->OnEvent(e);
		if (e.handled) {
			break;
		}
	}
}

void ViewSystem::Update(float deltaTime) {
	for (auto &layer : m_Layers) {
		layer->Update(deltaTime);
	}
}

void ViewSystem::Compute() {
	for (auto &layer : m_Layers) {
		layer->Compute();
	}
}

void ViewSystem::RenderLayers(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd, UILayer from, UILayer to) {
	const int first = static_cast<int>(from);
	const int last = static_cast<int>(to);
	for (int i = first; i <= last; i++) {
		m_Layers[i]->SubmitToQuadBatcher(r2d, cmd);
	}
}

bool ViewSystem::IsAnyLayerDirty(UILayer from, UILayer to) const {
	for (int i = static_cast<int>(from); i <= static_cast<int>(to); i++) {
		if (m_Layers[i]->IsDrawListDirty()) {
			return true;
		}
	}
	return false;
}

void ViewSystem::ClearLayerDirtyFlags(UILayer from, UILayer to) {
	for (int i = static_cast<int>(from); i <= static_cast<int>(to); i++) {
		m_Layers[i]->ClearDrawListDirty();
	}
}

void ViewSystem::Resize(uint32 width, uint32 height) {
	for (auto &layer : m_Layers) {
		layer->Resize(width, height);
	}
}

} // namespace Aquila::UI::Core
