#include "Aquila/UI/Core/CanvasManager.h"

namespace Aquila::UI::Core {

CanvasManager::CanvasManager(uint32 width, uint32 height) {
	for (auto &layer : m_Layers) {
		layer = CreateUnique<Canvas>(width, height);
	}
}

Canvas &CanvasManager::GetLayer(UILayer layer) {
	return *m_Layers[static_cast<uint8>(layer)];
}

void CanvasManager::OnEvent(Application::Events::Event &e) {
	for (int i = static_cast<int>(m_Layers.size()) - 1; i >= 0; i--) {
		m_Layers[i]->OnEvent(e);
		if (e.handled) {
			break;
		}
	}
}

void CanvasManager::Update(float deltaTime) {
	for (auto &layer : m_Layers) {
		layer->Update(deltaTime);
	}
}

void CanvasManager::Compute() {
	for (auto &layer : m_Layers) {
		layer->Compute();
	}
}

void CanvasManager::RenderLayers(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd, UILayer from, UILayer to) {
	const int first = static_cast<int>(from);
	const int last = static_cast<int>(to);
	for (int i = first; i <= last; i++) {
		m_Layers[i]->SubmitToQuadBatcher(r2d, cmd);
	}
}

bool CanvasManager::IsAnyLayerDirty(UILayer from, UILayer to) const {
	for (int i = static_cast<int>(from); i <= static_cast<int>(to); i++) {
		if (m_Layers[i]->IsDrawListDirty()) {
			return true;
		}
	}
	return false;
}

void CanvasManager::ClearLayerDirtyFlags(UILayer from, UILayer to) {
	for (int i = static_cast<int>(from); i <= static_cast<int>(to); i++) {
		m_Layers[i]->ClearDrawListDirty();
	}
}

void CanvasManager::Resize(uint32 width, uint32 height) {
	for (auto &layer : m_Layers) {
		layer->Resize(width, height);
	}
}

} // namespace Aquila::UI::Core
