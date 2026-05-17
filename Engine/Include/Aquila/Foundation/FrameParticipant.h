#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Foundation {

class IFrameParticipant {
  public:
	virtual ~IFrameParticipant() = default;

	virtual void OnUpdate(f32 deltaTime) {}
	virtual void OnCompute() {}
	virtual void OnPrepareRender() {}
	virtual void OnPostFrame() {}

  protected:
	IFrameParticipant() = default;
	AQUILA_NONCOPYABLE(IFrameParticipant);
};

} // namespace Aquila::Foundation
