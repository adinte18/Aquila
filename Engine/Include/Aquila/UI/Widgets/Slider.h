#pragma once

#include "Aquila/UI/Widgets/BaseField.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class Slider : public BaseField<float> {
  public:
	Slider();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Slider"; }

	void SetRange(float min, float max);
	void SetStep(float step);
	void SetTrackTexture(GFX::GfxTexture *tex) {
		m_TrackTex = tex;
		QueueRedraw();
	}

	[[nodiscard]] float GetMin() const { return m_Min; }
	[[nodiscard]] float GetMax() const { return m_Max; }

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  protected:
	float Coerce(const float &value) const override;
	void OnValueUpdated() override { QueueRedraw(); }

  private:
	float ValueFromX(float x) const;

	float m_Min = 0.f;
	float m_Max = 1.f;
	float m_Step = 0.f; // 0 = continuous

	GFX::GfxTexture *m_TrackTex = nullptr;
};

} // namespace Aquila::UI::Core
