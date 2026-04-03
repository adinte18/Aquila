#ifndef AQUILA_IRHI_PIPELINE_H
#define AQUILA_IRHI_PIPELINE_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHICommandList;

class IRHIPipeline {
  public:
	virtual ~IRHIPipeline() = default;

	IRHIPipeline(const IRHIPipeline &) = delete;
	IRHIPipeline &operator=(const IRHIPipeline &) = delete;

	virtual void Bind(IRHICommandList &cmd) = 0;

  protected:
	IRHIPipeline() = default;
};

} // namespace Aquila::RHI
#endif
