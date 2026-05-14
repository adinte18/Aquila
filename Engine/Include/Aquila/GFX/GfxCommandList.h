#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHICommandList.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::GFX {

class GfxContext;

class GfxCommandList {
  public:
	~GfxCommandList() = default;
	AQUILA_NONCOPYABLE(GfxCommandList);

	void Begin();
	void End();
	void Reset();

	[[nodiscard]] bool IsRecording() const;
	[[nodiscard]] RHI::CommandListType GetType() const;
	[[nodiscard]] const std::string &GetName() const;

	void TransitionTexture(GfxTexture &texture, RHI::ResourceState oldState, RHI::ResourceState newState);
	void TransitionBuffer(GfxBuffer &buffer, RHI::ResourceState oldState, RHI::ResourceState newState);

	void BindPipeline(GfxPipeline &pipeline);
	void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
	void SetScissor(int32 x, int32 y, uint32 width, uint32 height);

	void BindDescriptorSet(uint32 set, GfxDescriptorSet &descriptorSet);

	template <typename T>
	void PushConstants(const T &data, RHI::ShaderStageFlags stages = RHI::ShaderStageFlags::Vertex, uint32 offset = 0) {
		m_Cmd->PushConstants(&data, sizeof(T), stages, offset);
	}

	void BindVertexBuffer(GfxBuffer &buf, uint32 binding = 0, uint64 offset = 0);
	void BindIndexBuffer(GfxBuffer &buf, RHI::IndexFormat fmt = RHI::IndexFormat::UInt32, uint64 offset = 0);

	void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0);
	void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 vertexOffset = 0,
					 uint32 firstInstance = 0);
	void DrawIndirect(GfxBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride);
	void DrawIndexedIndirect(GfxBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride);

	void CopyBufferToTexture(GfxBuffer &src, GfxTexture &dst, uint32 width, uint32 height,
							 uint32 dstArrayLayer = 0, uint32 dstMipLevel = 0);

	void Dispatch(uint32 x, uint32 y, uint32 z);

	void PushDebugGroup(const char *name);
	void PopDebugGroup();

	[[nodiscard]] RHI::IRHICommandList &GetRHI() { return *m_Cmd; }

  private:
	friend class GfxContext;
	explicit GfxCommandList(Unique<RHI::IRHICommandList> cmd);

	Unique<RHI::IRHICommandList> m_Cmd;
};

} // namespace Aquila::GFX
