#pragma once

#include "Aquila/UI/Widgets/Control.h"
#include "Aquila/UI/Core/TextInputState.h"
#include "Aquila/UI/Text/FontAtlas.h"

namespace Aquila::UI::Core {

class DragFloat : public Control {
  public:
	struct Config {
		float min = -1e18f;
		float max = 1e18f;
		float step = 0.f;
		float speed = 1.f;
		int precision = 3;
		std::string prefix;
	};

	DragFloat();
	explicit DragFloat(const Config &config);

	[[nodiscard]] std::string_view GetTypeName() const override { return "DragFloat"; }

	void SetValue(float value);
	void SetRange(float min, float max);
	void SetStep(float step);
	void SetSpeed(float pixelsPerUnit);
	void SetPrecision(int decimals);
	void SetPrefix(std::string prefix);
	Signal<void(float)> onChanged;

	[[nodiscard]] float GetValue() const { return m_Value; }

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;
	void OnKeyPress(Platform::KeyCode key, int mods = 0) override;
	void OnCharInput(uint32 codepoint) override;
	void OnFocusLost() override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  protected:
	[[nodiscard]] virtual std::string FormatValue() const;

	virtual void OnValueCommitted() {}

	float m_Value = 0.f;
	float m_Min = -1e18f;
	float m_Max = 1e18f;
	float m_Step = 0.f;
	float m_Speed = 1.0f;
	int m_Precision = 3;
	std::string m_Prefix;

  private:
	enum class Mode { Drag, Edit };

	void EnterEditMode();
	void CommitEdit();
	void CancelEdit();
	[[nodiscard]] Text::FontAtlas *ResolveFont() const;

	Mode m_Mode = Mode::Drag;
	TextInputState m_EditState;
	float m_DragStartValue = 0.f;
	float m_DragStartX = 0.f;
	bool m_HasDragged = false;

	static constexpr float kDragThreshold = 3.f;

};

} // namespace Aquila::UI::Core
