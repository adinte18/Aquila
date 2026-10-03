#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Foundation/Timer.h"

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

	void request_frame_in(F64 seconds, Target target = nullptr) {
		const TimePoint at =
			now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<F64>(std::max(seconds, 0.0)));
		for (Deadline &deadline : m_deadlines) {
			if (deadline.target == target) {
				deadline.at = std::min(deadline.at, at);
				return;
			}
		}
		m_deadlines.push_back({ .target = target, .at = at });
	}

	[[nodiscard]] bool is_pending() const {
		if (m_dirty || !m_targets.empty()) {
			return true;
		}
		const TimePoint current = now();
		return std::ranges::any_of(m_deadlines, [current](const Deadline &deadline) { return deadline.at <= current; });
	}

	[[nodiscard]] Option<F64> seconds_until_deadline() const {
		if (m_deadlines.empty()) {
			return std::nullopt;
		}
		const auto earliest = std::ranges::min_element(m_deadlines, {}, &Deadline::at);
		return std::max(elapsed_seconds(now(), earliest->at), 0.0);
	}

	bool consume() {
		promote_due_deadlines();
		if (!m_dirty) {
			return false;
		}
		m_dirty = false;
		return true;
	}

	bool consume(Target target) {
		promote_due_deadlines();
		const auto it = std::ranges::find(m_targets, target);
		if (it == m_targets.end()) {
			return false;
		}
		m_targets.erase(it);
		return true;
	}

	void clear_targets() { m_targets.clear(); }

	void forget(Target target) {
		std::erase(m_targets, target);
		std::erase_if(m_deadlines, [target](const Deadline &deadline) { return deadline.target == target; });
	}

  private:
	friend class Singleton<FrameScheduler>;
	FrameScheduler() = default;

	struct Deadline {
		Target target = nullptr;
		TimePoint at;
	};

	void promote_due_deadlines() {
		const TimePoint current = now();
		std::erase_if(m_deadlines, [this, current](const Deadline &deadline) {
			if (deadline.at > current) {
				return false;
			}
			if (deadline.target == nullptr) {
				m_dirty = true;
			} else if (std::ranges::find(m_targets, deadline.target) == m_targets.end()) {
				m_targets.push_back(deadline.target);
			}
			return true;
		});
	}

	bool m_dirty = true; // always render the first frame
	std::vector<Target> m_targets;
	std::vector<Deadline> m_deadlines;
};

} // namespace Aquila::Foundation
