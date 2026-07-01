#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"

template <typename Sig> class Signal;

template <typename... Args> class Signal<void(Args...)> {
	using Slot = Delegate<void(Args...)>;
	std::vector<Slot> m_Slots;

  public:
	void Connect(Slot slot) { m_Slots.push_back(std::move(slot)); }

	void Set(Slot slot) {
		m_Slots.clear();
		m_Slots.push_back(std::move(slot));
	}

	void Clear() { m_Slots.clear(); }

	void operator()(Args... args) const {
		for (const auto &slot : m_Slots) {
			slot(args...);
		}
	}

	[[nodiscard]] bool HasConnections() const { return !m_Slots.empty(); }
};
