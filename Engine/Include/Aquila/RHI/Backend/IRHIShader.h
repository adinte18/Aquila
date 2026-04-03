#ifndef AQUILA_IRHI_SHADER_H
#define AQUILA_IRHI_SHADER_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHIShader {
  public:
	virtual ~IRHIShader() = default;

	IRHIShader(const IRHIShader &) = delete;
	IRHIShader &operator=(const IRHIShader &) = delete;

	[[nodiscard]] virtual ShaderStageFlags GetStage() const = 0;
	[[nodiscard]] virtual const std::string &GetEntryPoint() const = 0;

  protected:
	IRHIShader() = default;
};

} // namespace Aquila::RHI
#endif
