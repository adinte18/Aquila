#ifndef AQUILA_IRHI_PIPELINE_H
#define AQUILA_IRHI_PIPELINE_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHICommandList;

enum class PipelineBindPoint : Uint8 { Graphics, Compute };

class IRHIPipeline {
  public:
	virtual ~IRHIPipeline() = default;

	IRHIPipeline(const IRHIPipeline &) = delete;
	IRHIPipeline &operator=(const IRHIPipeline &) = delete;

	virtual void bind(IRHICommandList &cmd) = 0;
	[[nodiscard]] virtual PipelineBindPoint get_bind_point() const = 0;

  protected:
	IRHIPipeline() = default;
};

} // namespace Aquila::RHI
#endif
