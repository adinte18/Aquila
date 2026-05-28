#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class Slider : public View {
  public:
	Slider();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Slider"; }

	void SetValue(float value);
	void SetRange(float min, float max);
	void SetStep(float step);
	void SetOnChanged(Delegate<void(float)> callback);
	void SetTrackTexture(GFX::GfxTexture *tex) {
		m_TrackTex = tex;
		QueueRedraw();
	}

	[[nodiscard]] float GetValue() const { return m_Value; }
	[[nodiscard]] float GetMin() const { return m_Min; }
	[[nodiscard]] float GetMax() const { return m_Max; }

	void OnMouseEnter() override;
	void OnMouseLeave() override;
	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  private:
	float ValueFromX(float x) const;

	float m_Value = 0.f;
	float m_Min = 0.f;
	float m_Max = 1.f;
	float m_Step = 0.f; // 0 = continuous

	Delegate<void(float)> m_OnChanged;
	GFX::GfxTexture *m_TrackTex = nullptr;
};

} // namespace Aquila::UI::Core
