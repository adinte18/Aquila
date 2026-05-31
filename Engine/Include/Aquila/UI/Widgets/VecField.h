#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/Label.h"

namespace Aquila::UI::Core {

class LabeledDragFloat : public View {
  public:
	explicit LabeledDragFloat(const char *label);

	DragFloat *GetDrag() { return m_Drag; }
	Label *GetLabel() { return m_Label; }

  private:
	Label *m_Label = nullptr;
	DragFloat *m_Drag = nullptr;
};

class Vec2Field : public View {
  public:
	Vec2Field();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Vec2Field"; }

	void SetValue(vec2 v);
	[[nodiscard]] vec2 GetValue() const;
	void SetOnChanged(Delegate<void(vec2)> callback);
	void SetStep(float step);
	void SetSpeed(float speed);

  private:
	DragFloat *m_X = nullptr;
	DragFloat *m_Y = nullptr;
	Delegate<void(vec2)> m_OnChanged;
};

class Vec3Field : public View {
  public:
	Vec3Field();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Vec3Field"; }

	void SetValue(vec3 v);
	[[nodiscard]] vec3 GetValue() const;
	void SetOnChanged(Delegate<void(vec3)> callback);
	void SetStep(float step);
	void SetSpeed(float speed);

  private:
	DragFloat *m_X = nullptr;
	DragFloat *m_Y = nullptr;
	DragFloat *m_Z = nullptr;
	Delegate<void(vec3)> m_OnChanged;
};

class Vec4Field : public View {
  public:
	Vec4Field();

	[[nodiscard]] std::string_view GetTypeName() const override { return "Vec4Field"; }

	void SetValue(vec4 v);
	[[nodiscard]] vec4 GetValue() const;
	void SetOnChanged(Delegate<void(vec4)> callback);
	void SetStep(float step);
	void SetSpeed(float speed);

  private:
	DragFloat *m_X = nullptr;
	DragFloat *m_Y = nullptr;
	DragFloat *m_Z = nullptr;
	DragFloat *m_W = nullptr;
	Delegate<void(vec4)> m_OnChanged;
};

} // namespace Aquila::UI::Core
