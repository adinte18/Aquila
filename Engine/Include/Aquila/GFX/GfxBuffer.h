#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIBuffer.h"

namespace Aquila::GFX {

class GfxContext;

class GfxBuffer {
  public:
	~GfxBuffer() = default;
	AQUILA_NONCOPYABLE(GfxBuffer);

	void Write(const void *data, uint64 size = 0, uint64 offset = 0);
	void *Map();
	void Unmap();
	void Flush(uint64 size = 0, uint64 offset = 0);

	void DestroyImmediate();

	[[nodiscard]] uint64 GetSize() const;
	[[nodiscard]] uint32 GetInstanceCount() const;
	[[nodiscard]] bool IsMapped() const;
	[[nodiscard]] RHI::IRHIBuffer &GetRHI() { return *m_Buffer; }

  private:
	friend class GfxContext;
	explicit GfxBuffer(Unique<RHI::IRHIBuffer> buffer);
	Unique<RHI::IRHIBuffer> m_Buffer;
};

} // namespace Aquila::GFX
