#pragma once
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

class IRHICommandListPool {
  public:
	virtual ~IRHICommandListPool() = default;
	AQUILA_NONCOPYABLE(IRHICommandListPool);
	AQUILA_NONMOVEABLE(IRHICommandListPool);

	virtual IRHICommandList *allocate(CommandListType type, const std::string &name = "") = 0;
	virtual void free(IRHICommandList *cmd) = 0;
	virtual void reset() = 0;
	[[nodiscard]] virtual Uint32 get_frames_in_flight() const = 0;

  protected:
	IRHICommandListPool() = default;
};

} // namespace Aquila::RHI
