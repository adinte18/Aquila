#pragma once
#include "Aquila/Foundation/Singleton.h"

#include <algorithm>
#include <vector>

namespace Aquila::Foundation {

class FrameScheduler : public Singleton<FrameScheduler> {
  public:
	using Target = const void *;

	void request_frame() { m_dirty = true; }

	void request_frame(Target target) {
		if (target == nullptr) {
			request_frame();
			return;
		}
		if (std::ranges::find(m_targets, target) == m_targets.end()) {
			m_targets.push_back(target);
		}
	}

	[[nodiscard]] bool is_pending() const { return m_dirty || !m_targets.empty(); }

	bool consume() {
		if (!m_dirty) {
			return false;
		}
		m_dirty = false;
		return true;
	}

	bool consume(Target target) {
		const auto it = std::ranges::find(m_targets, target);
		if (it == m_targets.end()) {
			return false;
		}
		m_targets.erase(it);
		return true;
	}

	void clear_targets() { m_targets.clear(); }

	void forget(Target target) { std::erase(m_targets, target); }

  private:
	friend class Singleton<FrameScheduler>;
	FrameScheduler() = default;
	bool m_dirty = true; // always render the first frame
	std::vector<Target> m_targets;
};

} // namespace Aquila::Foundation
