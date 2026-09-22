#pragma once
#include "Aquila/Foundation/Singleton.h"

namespace Aquila::Foundation {

class FrameScheduler : public Singleton<FrameScheduler> {
  public:
	void request_frame() { m_dirty = true; }

	bool consume() {
		if (!m_dirty) {
			return false;
		}
		m_dirty = false;
		return true;
	}

  private:
	friend class Singleton<FrameScheduler>;
	FrameScheduler() = default;
	bool m_dirty = true; // always render the first frame
};

} // namespace Aquila::Foundation
