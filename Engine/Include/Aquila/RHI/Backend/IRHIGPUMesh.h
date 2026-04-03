#pragma once

#include "Aquila/RHI/Vertex.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

struct GPUMeshPrimitive {
	uint32 firstIndex = 0;
	uint32 firstVertex = 0;
	uint32 indexCount = 0;
	uint32 vertexCount = 0;
};

struct GPUMeshDesc {
	const std::vector<Vertex> *vertices = nullptr;
	const std::vector<uint32> *indices = nullptr;
	const std::vector<GPUMeshPrimitive> *primitives = nullptr;
	std::string debugName;
};

class IRHIGPUMesh {
  public:
	virtual ~IRHIGPUMesh() = default;
	AQUILA_NONCOPYABLE(IRHIGPUMesh);
	AQUILA_NONMOVEABLE(IRHIGPUMesh);

	virtual void Bind(IRHICommandList &cmd) const = 0;
	virtual void Draw(IRHICommandList &cmd) const = 0;

	[[nodiscard]] virtual bool IsValid() const = 0;
	[[nodiscard]] virtual uint32 GetVertexCount() const = 0;
	[[nodiscard]] virtual uint32 GetIndexCount() const = 0;
	[[nodiscard]] virtual bool HasIndexBuffer() const = 0;

  protected:
	IRHIGPUMesh() = default;
};

} // namespace Aquila::RHI
