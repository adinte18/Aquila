#ifndef AQUILA_IRHI_BUFFER_H
#define AQUILA_IRHI_BUFFER_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

class IRHIBuffer {
  public:
	virtual ~IRHIBuffer() = default;

	IRHIBuffer(const IRHIBuffer &) = delete;
	IRHIBuffer &operator=(const IRHIBuffer &) = delete;

	virtual void write(const void *data, Uint64 size, Uint64 offset = 0) = 0;
	virtual void *map() = 0;
	virtual void unmap() = 0;
	virtual void flush(Uint64 size = 0, Uint64 offset = 0) = 0;
	virtual void invalidate(Uint64 size = 0, Uint64 offset = 0) = 0;

	virtual void destroy_immediate() = 0;

	[[nodiscard]] virtual Uint64 get_size() const = 0;
	[[nodiscard]] virtual Uint32 get_instance_count() const = 0;
	[[nodiscard]] virtual bool is_mapped() const = 0;

  protected:
	IRHIBuffer() = default;
};

} // namespace Aquila::RHI
#endif
