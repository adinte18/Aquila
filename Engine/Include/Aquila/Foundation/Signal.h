#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"
#include <vector>

template <typename Sig> class Signal;

template <typename... Args> class Signal<void(Args...)> {
	using Slot = Delegate<void(Args...)>;
	std::vector<Slot> m_slots;

  public:
	void connect(Slot slot) { m_slots.push_back(std::move(slot)); }

	void set(Slot slot) {
		m_slots.clear();
		m_slots.push_back(std::move(slot));
	}

	void clear() { m_slots.clear(); }

	void operator()(Args... args) const {
		for (const auto &slot : m_slots) {
			slot(args...);
		}
	}

	[[nodiscard]] bool has_connections() const { return !m_slots.empty(); }
};
