#pragma once
#include "Aquila/Foundation/Singleton.h"

namespace Aquila::Rendering {

class FrameScheduler : public Foundation::Singleton<FrameScheduler> {
  public:
	void RequestFrame() { m_Dirty = true; }

	bool Consume() {
		if (!m_Dirty) {
			return false;
		}
		m_Dirty = false;
		return true;
	}

  private:
	friend class Foundation::Singleton<FrameScheduler>;
	FrameScheduler() = default;
	bool m_Dirty = true; // always render the first frame
};

} // namespace Aquila::Rendering
