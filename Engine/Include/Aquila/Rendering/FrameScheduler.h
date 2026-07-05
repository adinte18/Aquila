#pragma once
#include "Aquila/Foundation/Singleton.h"

namespace Aquila::Rendering {

class FrameScheduler : public Foundation::Singleton<FrameScheduler> {
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
	friend class Foundation::Singleton<FrameScheduler>;
	FrameScheduler() = default;
	bool m_dirty = true; // always render the first frame
};

} // namespace Aquila::Rendering
