#pragma once

#include "Aquila/UI/Widgets/Control.h"
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

template <int N, typename VecT> class VecFieldBase : public Control {
  public:
	void SetValue(VecT v) {
		for (int i = 0; i < N; ++i) {
			m_Components[i]->SetValue(v[i]);
		}
	}

	[[nodiscard]] VecT GetValue() const {
		VecT v{};
		for (int i = 0; i < N; ++i) {
			v[i] = m_Components[i]->GetValue();
		}
		return v;
	}

	void SetStep(float step) {
		for (int i = 0; i < N; ++i) {
			m_Components[i]->SetStep(step);
		}
	}

	void SetSpeed(float speed) {
		for (int i = 0; i < N; ++i) {
			m_Components[i]->SetSpeed(speed);
		}
	}

	Signal<void(VecT)> onChanged;

  protected:
	void RegisterComponent(int index, DragFloat *drag) {
		m_Components[index] = drag;
		drag->onChanged.Connect([this](float) { onChanged(GetValue()); });
	}

  private:
	DragFloat *m_Components[N] = {};
};

class Vec2Field : public VecFieldBase<2, vec2> {
  public:
	Vec2Field();
	[[nodiscard]] std::string_view GetTypeName() const override { return "Vec2Field"; }
};

class Vec3Field : public VecFieldBase<3, vec3> {
  public:
	Vec3Field();
	[[nodiscard]] std::string_view GetTypeName() const override { return "Vec3Field"; }
};

class Vec4Field : public VecFieldBase<4, vec4> {
  public:
	Vec4Field();
	[[nodiscard]] std::string_view GetTypeName() const override { return "Vec4Field"; }
};

} // namespace Aquila::UI::Core
