#pragma once

#include "Aquila/UI/Widgets/Control.h"

namespace Aquila::UI::Core {

template <typename T> class BaseField : public Control {
  public:
	[[nodiscard]] const T &GetValue() const { return m_Value; }

	void SetValue(const T &value) {
		const T coerced = Coerce(value);
		if (coerced == m_Value) {
			return;
		}
		m_Value = coerced;
		OnValueUpdated();
		onChanged(m_Value);
	}

	void SetValueWithoutNotify(const T &value) {
		m_Value = Coerce(value);
		OnValueUpdated();
	}

	Signal<void(const T &)> onChanged;

  protected:
	virtual void OnValueUpdated() {}

	virtual T Coerce(const T &value) const { return value; }

	T m_Value{};
};

} // namespace Aquila::UI::Core
