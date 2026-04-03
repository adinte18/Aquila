#pragma once
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

class IRHICommandListPool {
  public:
	virtual ~IRHICommandListPool() = default;
	AQUILA_NONCOPYABLE(IRHICommandListPool);
	AQUILA_NONMOVEABLE(IRHICommandListPool);

	virtual IRHICommandList *Allocate(CommandListType type, const std::string &name = "") = 0;
	virtual void Free(IRHICommandList *cmd) = 0;
	virtual void Reset() = 0;
	[[nodiscard]] virtual uint32 GetFramesInFlight() const = 0;

  protected:
	IRHICommandListPool() = default;
};

} // namespace Aquila::RHI
