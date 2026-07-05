#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/DockTypes.h"

namespace Aquila::UI::Core {

class DockSplitter : public View {
  public:
	explicit DockSplitter(SplitDirection dir);

	[[nodiscard]] std::string_view get_type_name() const override { return "DockSplitter"; }

	void set_siblings(View *before, View *after);
	void update_sibling_ref(View *old, View *new_ptr);
	void set_resize_before(bool v) { m_resize_before = v; }

	[[nodiscard]] View *get_before() const { return m_before; }
	[[nodiscard]] View *get_after() const { return m_after; }

	void on_mouse_press(Platform::MouseButton btn, Vec2 pos) override;
	void on_mouse_release(Platform::MouseButton btn, Vec2 pos) override;
	void on_mouse_move(Vec2 pos) override;

  private:
	SplitDirection m_dir;
	View *m_before = nullptr;
	View *m_after = nullptr;
	bool m_resize_before = true;
	Vec2 m_drag_start_pos{};
	float m_before_grow_start = 0.F;
	float m_after_grow_start = 0.F;
};

} // namespace Aquila::UI::Core
