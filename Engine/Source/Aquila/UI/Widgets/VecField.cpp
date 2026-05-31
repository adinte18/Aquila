#include "Aquila/UI/Widgets/VecField.h"

namespace Aquila::UI::Core {

LabeledDragFloat::LabeledDragFloat(const char *label) {
	AddClass("labeled-drag");

	auto lbl = CreateUnique<Label>(label);
	lbl->AddClass("vec-label");
	m_Label = static_cast<Label *>(AddChild(std::move(lbl)));

	auto drag = CreateUnique<DragFloat>();
	drag->AddClass("vec-drag");
	m_Drag = static_cast<DragFloat *>(AddChild(std::move(drag)));
}

Vec2Field::Vec2Field() {
	AddClass("vec2-field");

	auto xf = CreateUnique<LabeledDragFloat>("X");
	auto yf = CreateUnique<LabeledDragFloat>("Y");

	m_X = xf->GetDrag();
	m_Y = yf->GetDrag();

	m_X->SetOnChanged([this](float) {
		if (m_OnChanged) {
			m_OnChanged(GetValue());
		}
	});
	m_Y->SetOnChanged([this](float) {
		if (m_OnChanged) {
			m_OnChanged(GetValue());
		}
	});

	AddChild(std::move(xf));
	AddChild(std::move(yf));
}

void Vec2Field::SetValue(vec2 v) {
	m_X->SetValue(v.x);
	m_Y->SetValue(v.y);
}

vec2 Vec2Field::GetValue() const {
	return { m_X->GetValue(), m_Y->GetValue() };
}

void Vec2Field::SetOnChanged(Delegate<void(vec2)> callback) {
	m_OnChanged = std::move(callback);
}

void Vec2Field::SetStep(float step) {
	m_X->SetStep(step);
	m_Y->SetStep(step);
}

void Vec2Field::SetSpeed(float speed) {
	m_X->SetSpeed(speed);
	m_Y->SetSpeed(speed);
}

Vec3Field::Vec3Field() {
	auto makeDrag = [](const char *prefix, const char *axisClass) {
		auto drag = CreateUnique<DragFloat>();
		drag->SetPrefix(prefix);
		drag->AddClass("vec-drag");
		drag->AddClass(axisClass);
		return drag;
	};

	auto xDrag = makeDrag("X ", "vec-drag-x");
	auto yDrag = makeDrag("Y ", "vec-drag-y");
	auto zDrag = makeDrag("Z ", "vec-drag-z");

	auto notify = [this](float) {
		if (m_OnChanged) {
			m_OnChanged(GetValue());
		}
	};
	xDrag->SetOnChanged(notify);
	yDrag->SetOnChanged(notify);
	zDrag->SetOnChanged(notify);

	m_X = static_cast<DragFloat *>(AddChild(std::move(xDrag)));
	m_Y = static_cast<DragFloat *>(AddChild(std::move(yDrag)));
	m_Z = static_cast<DragFloat *>(AddChild(std::move(zDrag)));
}

void Vec3Field::SetValue(vec3 v) {
	m_X->SetValue(v.x);
	m_Y->SetValue(v.y);
	m_Z->SetValue(v.z);
}

vec3 Vec3Field::GetValue() const {
	return { m_X->GetValue(), m_Y->GetValue(), m_Z->GetValue() };
}

void Vec3Field::SetOnChanged(Delegate<void(vec3)> callback) {
	m_OnChanged = std::move(callback);
}

void Vec3Field::SetStep(float step) {
	m_X->SetStep(step);
	m_Y->SetStep(step);
	m_Z->SetStep(step);
}

void Vec3Field::SetSpeed(float speed) {
	m_X->SetSpeed(speed);
	m_Y->SetSpeed(speed);
	m_Z->SetSpeed(speed);
}

Vec4Field::Vec4Field() {
	AddClass("vec4-field");

	auto xf = CreateUnique<LabeledDragFloat>("X");
	auto yf = CreateUnique<LabeledDragFloat>("Y");
	auto zf = CreateUnique<LabeledDragFloat>("Z");
	auto wf = CreateUnique<LabeledDragFloat>("W");

	m_X = xf->GetDrag();
	m_Y = yf->GetDrag();
	m_Z = zf->GetDrag();
	m_W = wf->GetDrag();

	auto notify = [this](float) {
		if (m_OnChanged) {
			m_OnChanged(GetValue());
		}
	};
	m_X->SetOnChanged(notify);
	m_Y->SetOnChanged(notify);
	m_Z->SetOnChanged(notify);
	m_W->SetOnChanged(notify);

	AddChild(std::move(xf));
	AddChild(std::move(yf));
	AddChild(std::move(zf));
	AddChild(std::move(wf));
}

void Vec4Field::SetValue(vec4 v) {
	m_X->SetValue(v.x);
	m_Y->SetValue(v.y);
	m_Z->SetValue(v.z);
	m_W->SetValue(v.w);
}

vec4 Vec4Field::GetValue() const {
	return { m_X->GetValue(), m_Y->GetValue(), m_Z->GetValue(), m_W->GetValue() };
}

void Vec4Field::SetOnChanged(Delegate<void(vec4)> callback) {
	m_OnChanged = std::move(callback);
}

void Vec4Field::SetStep(float step) {
	m_X->SetStep(step);
	m_Y->SetStep(step);
	m_Z->SetStep(step);
	m_W->SetStep(step);
}

void Vec4Field::SetSpeed(float speed) {
	m_X->SetSpeed(speed);
	m_Y->SetSpeed(speed);
	m_Z->SetSpeed(speed);
	m_W->SetSpeed(speed);
}

} // namespace Aquila::UI::Core
