#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Foundation {

class IFrameParticipant {
  public:
	virtual ~IFrameParticipant() = default;

	virtual void on_update(F32 delta_time) {}
	virtual void on_compute() {}
	virtual void on_prepare_render() {}
	virtual void on_post_frame() {}

  protected:
	IFrameParticipant() = default;
	AQUILA_NONCOPYABLE(IFrameParticipant);
};

} // namespace Aquila::Foundation
