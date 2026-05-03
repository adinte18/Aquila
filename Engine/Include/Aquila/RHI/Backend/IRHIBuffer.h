#ifndef AQUILA_IRHI_BUFFER_H
#define AQUILA_IRHI_BUFFER_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHIBuffer {
  public:
	virtual ~IRHIBuffer() = default;

	IRHIBuffer(const IRHIBuffer &) = delete;
	IRHIBuffer &operator=(const IRHIBuffer &) = delete;

	virtual void Write(const void *data, uint64 size, uint64 offset = 0) = 0;
	virtual void *Map() = 0;
	virtual void Unmap() = 0;
	virtual void Flush(uint64 size = 0, uint64 offset = 0) = 0;

	virtual void DestroyImmediate() = 0;

	[[nodiscard]] virtual uint64 GetSize() const = 0;
	[[nodiscard]] virtual uint32 GetInstanceCount() const = 0;
	[[nodiscard]] virtual bool IsMapped() const = 0;

  protected:
	IRHIBuffer() = default;
};

} // namespace Aquila::RHI
#endif
