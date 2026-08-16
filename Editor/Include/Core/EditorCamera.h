#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Rendering/Camera.h"
#include "Aquila/Rendering/RenderView.h"

namespace Aquila::Application::Events {
class Event;
}

namespace Editor {

class EditorCamera {
  public:
	EditorCamera();

	void set_viewport_size(Uint32 width, Uint32 height);
	void set_viewport_rect(Vec2 position, Vec2 size);

	void update(F32 delta_time);
	void on_event(Aquila::Application::Events::Event &event);

	[[nodiscard]] Aquila::Rendering::RenderView get_render_view() const;

  private:
	enum class NavMode { None, Fly, Orbit };

	Aquila::Rendering::Camera m_camera;

	F32 m_fov = 60.F;
	F32 m_near = 0.1F;
	F32 m_far = 1000.F;
	F32 m_aspect = 16.F / 9.F;

	Vec2 m_viewport_pos{ 0.F, 0.F };
	Vec2 m_viewport_size{ 0.F, 0.F };
	Uint32 m_width = 0;
	Uint32 m_height = 0;

	NavMode m_nav_mode = NavMode::None;
	Vec2 m_last_mouse{ 0.F, 0.F };
	bool m_prev_rmb = false;
	bool m_prev_lmb = false;
};

} // namespace Editor
