#pragma once

#include "Aquila/UI/Widgets/BaseField.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class Slider : public BaseField<float> {
  public:
	Slider();

	[[nodiscard]] std::string_view get_type_name() const override { return "Slider"; }

	void set_range(float min, float max);
	void set_step(float step);
	void set_track_texture(GFX::GfxTexture *tex) {
		m_track_tex = tex;
		queue_redraw();
	}

	[[nodiscard]] float get_min() const { return m_min; }
	[[nodiscard]] float get_max() const { return m_max; }

	void on_mouse_press(Platform::MouseButton btn, Vec2 pos) override;
	void on_mouse_move(Vec2 pos) override;
	void on_draw_self(Rendering::DrawList &draw_list) override;

  protected:
	float coerce(const float &value) const override;
	void on_value_updated() override { queue_redraw(); }

  private:
	float value_from_x(float x) const;

	float m_min = 0.F;
	float m_max = 1.F;
	float m_step = 0.F; // 0 = continuous

	GFX::GfxTexture *m_track_tex = nullptr;
};

} // namespace Aquila::UI::Core
