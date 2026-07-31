#pragma once

#include "Aquila/Graphics/Core/QuadBatcher.h"
#include "Aquila/GFX/GfxCommandList.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/Foundation/Singleton.h"

namespace Aquila::UI::Core {

enum class UILayer : Uint8 {
	WorldSpace = 0,
	ScreenCamera = 1,
	ScreenOverlay = 2,
	Editor = 3,
	Count // keep track of how many layers we have
};

class CanvasManager : public Foundation::Singleton<CanvasManager> {
  public:
	Canvas &get_layer(UILayer layer);

	void on_event(Application::Events::Event &e);
	void update(float delta_time);
	void compute();

	void render_layers(Graphics::QuadBatcher &r2d, GFX::GfxCommandList &cmd, UILayer from, UILayer to);
	void resize(Uint32 width, Uint32 height);

	bool is_any_layer_dirty(UILayer from, UILayer to) const;
	void clear_layer_dirty_flags(UILayer from, UILayer to);

	[[nodiscard]] Platform::CursorType get_active_cursor() const {
		for (size_t i = static_cast<size_t>(UILayer::Count); i-- > 0;) {
			if (View *hovered = m_layers[i]->get_hovered_view()) {
				return hovered->get_cursor();
			}
		}
		return Platform::CursorType::Arrow;
	}

  private:
	friend class Foundation::Singleton<CanvasManager>;
	CanvasManager(Uint32 width, Uint32 height);

	std::array<Unique<Canvas>, static_cast<size_t>(UILayer::Count)> m_layers;
};

} // namespace Aquila::UI::Core
