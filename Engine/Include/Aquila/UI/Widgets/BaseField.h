#pragma once

#include "Aquila/UI/Widgets/Control.h"

namespace Aquila::UI::Core {

template <typename T> class BaseField : public Control {
  public:
	[[nodiscard]] const T &get_value() const { return m_value; }

	void set_value(const T &value) {
		const T coerced = coerce(value);
		if (coerced == m_value) {
			return;
		}
		m_value = coerced;
		on_value_updated();
		on_changed(m_value);
	}

	void set_value_without_notify(const T &value) {
		m_value = coerce(value);
		on_value_updated();
	}

	Signal<void(const T &)> on_changed;

  protected:
	virtual void on_value_updated() {}

	virtual T coerce(const T &value) const { return value; }

	T m_value{};
};

} // namespace Aquila::UI::Core
