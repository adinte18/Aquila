#include "Aquila/UI/Core/UIHost.h"

#include "Aquila/Platform/Events/Event.h"
#include "Aquila/Rendering/Renderers/Renderer2D.h"
#include "Aquila/UI/Core/Clipboard.h"
#include "Aquila/UI/Rendering/ViewRenderingSystem.h"

namespace Aquila::UI::Core {

UIHost::UIHost(const UIHostDesc &desc) {
	CanvasManager::init(desc.width, desc.height);
	desc.renderer_2d.add_system<Rendering::ViewRenderingSystem>();
	Clipboard::init(desc.clipboard_get, desc.clipboard_set);
}

UIHost::~UIHost() {
	CanvasManager::shutdown();
}

void UIHost::update(F32 delta_time) {
	CanvasManager::get()->update(delta_time);
	CanvasManager::get()->compute();
}

void UIHost::on_event(Platform::Events::Event &event) {
	CanvasManager::get()->on_event(event);
}

}
