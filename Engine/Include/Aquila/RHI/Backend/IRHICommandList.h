#ifndef AQUILA_IRHI_COMMAND_LIST_H
#define AQUILA_IRHI_COMMAND_LIST_H

#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/Backend/IRHIBuffer.h"
#include "Aquila/RHI/Backend/IRHITexture.h"
#include "Aquila/RHI/Backend/IRHIDescriptors.h"
#include "Aquila/RHI/Backend/IRHIPipeline.h"

namespace Aquila::RHI {

class IRHICommandList {
  public:
	virtual ~IRHICommandList() = default;

	IRHICommandList(const IRHICommandList &) = delete;
	IRHICommandList &operator=(const IRHICommandList &) = delete;

	virtual void Begin() = 0;
	virtual void End() = 0;
	virtual void Reset() = 0;

	[[nodiscard]] virtual bool IsRecording() const = 0;
	[[nodiscard]] virtual CommandListType GetType() const = 0;
	[[nodiscard]] virtual const std::string &GetName() const = 0;

	// The caller is responsible for tracking old/new states; no internal state is
	// kept by the command list.  Both states use the engine's ResourceState bitmask.
	// Emitting oldState == newState is a no-op on all backends.

	virtual void TransitionTexture(IRHITexture &texture, ResourceState oldState, ResourceState newState) = 0;
	virtual void TransitionBuffer(IRHIBuffer &buffer, ResourceState oldState, ResourceState newState) = 0;

	// Binding a pipeline also captures the pipeline layout, which is required for
	// subsequent BindDescriptorSet and PushConstants calls.

	virtual void BindPipeline(IRHIPipeline &pipeline) = 0;
	virtual void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f,
							 float maxDepth = 1.0f) = 0;
	virtual void SetScissor(int32 x, int32 y, uint32 width, uint32 height) = 0;

	virtual void BindDescriptorSet(uint32 set, IRHIDescriptorSet &descriptorSet) = 0;
	virtual void PushConstants(const void *data, uint32 size, ShaderStageFlags stages, uint32 offset = 0) = 0;
	virtual void BindVertexBuffer(IRHIBuffer &buffer, uint32 binding = 0, uint64 offset = 0) = 0;
	virtual void BindIndexBuffer(IRHIBuffer &buffer, IndexFormat format = IndexFormat::UInt32, uint64 offset = 0) = 0;

	virtual void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0,
					  uint32 firstInstance = 0) = 0;
	virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 vertexOffset = 0,
							 uint32 firstInstance = 0) = 0;
	virtual void DrawIndirect(IRHIBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) = 0;
	virtual void DrawIndexedIndirect(IRHIBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) = 0;

	virtual void PushDebugGroup(const char *name) = 0;
	virtual void PopDebugGroup() = 0;

  protected:
	IRHICommandList() = default;
};

} // namespace Aquila::RHI
#endif
