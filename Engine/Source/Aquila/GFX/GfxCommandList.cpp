#include "Aquila/GFX/GfxCommandList.h"

namespace Aquila::GFX {

GfxCommandList::GfxCommandList(Unique<RHI::IRHICommandList> cmd) : m_Cmd(std::move(cmd)) {}

void GfxCommandList::Begin() {
	m_Cmd->Begin();
}
void GfxCommandList::End() {
	m_Cmd->End();
}
void GfxCommandList::Reset() {
	m_Cmd->Reset();
}

bool GfxCommandList::IsRecording() const {
	return m_Cmd->IsRecording();
}
RHI::CommandListType GfxCommandList::GetType() const {
	return m_Cmd->GetType();
}
const std::string &GfxCommandList::GetName() const {
	return m_Cmd->GetName();
}

void GfxCommandList::TransitionTexture(GfxTexture &texture, RHI::ResourceState oldState, RHI::ResourceState newState) {
	m_Cmd->TransitionTexture(texture.GetRHI(), oldState, newState);
}

void GfxCommandList::TransitionBuffer(GfxBuffer &buffer, RHI::ResourceState oldState, RHI::ResourceState newState) {
	m_Cmd->TransitionBuffer(buffer.GetRHI(), oldState, newState);
}

void GfxCommandList::BindPipeline(GfxPipeline &pipeline) {
	m_Cmd->BindPipeline(pipeline.GetRHI());
}

void GfxCommandList::SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) {
	m_Cmd->SetViewport(x, y, width, height, minDepth, maxDepth);
}

void GfxCommandList::SetScissor(int32 x, int32 y, uint32 width, uint32 height) {
	m_Cmd->SetScissor(x, y, width, height);
}

void GfxCommandList::BindDescriptorSet(uint32 set, GfxDescriptorSet &descriptorSet) {
	m_Cmd->BindDescriptorSet(set, descriptorSet.GetRHI());
}

void GfxCommandList::BindVertexBuffer(GfxBuffer &buf, uint32 binding, uint64 offset) {
	m_Cmd->BindVertexBuffer(buf.GetRHI(), binding, offset);
}

void GfxCommandList::BindIndexBuffer(GfxBuffer &buf, RHI::IndexFormat fmt, uint64 offset) {
	m_Cmd->BindIndexBuffer(buf.GetRHI(), fmt, offset);
}

void GfxCommandList::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) {
	m_Cmd->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void GfxCommandList::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset,
								 uint32 firstInstance) {
	m_Cmd->DrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void GfxCommandList::DrawIndirect(GfxBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) {
	m_Cmd->DrawIndirect(buffer.GetRHI(), offset, drawCount, stride);
}

void GfxCommandList::DrawIndexedIndirect(GfxBuffer &buffer, uint64 offset, uint32 drawCount, uint32 stride) {
	m_Cmd->DrawIndexedIndirect(buffer.GetRHI(), offset, drawCount, stride);
}

void GfxCommandList::CopyBufferToTexture(GfxBuffer &src, GfxTexture &dst, uint32 width, uint32 height,
										 uint32 dstArrayLayer, uint32 dstMipLevel) {
	m_Cmd->CopyBufferToTexture(src.GetRHI(), dst.GetRHI(), width, height, dstArrayLayer, dstMipLevel);
}

void GfxCommandList::FillBuffer(GfxBuffer &buffer, uint32 value, uint64 offset, uint64 size) {
	m_Cmd->FillBuffer(buffer.GetRHI(), offset, size, value);
}

void GfxCommandList::Dispatch(uint32 x, uint32 y, uint32 z) {
	m_Cmd->Dispatch(x, y, z);
}

void GfxCommandList::PushDebugGroup(const char *name) {
	m_Cmd->PushDebugGroup(name);
}
void GfxCommandList::PopDebugGroup() {
	m_Cmd->PopDebugGroup();
}

} // namespace Aquila::GFX
