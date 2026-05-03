#include "Aquila/GFX/GfxBuffer.h"

namespace Aquila::GFX {

GfxBuffer::GfxBuffer(Unique<RHI::IRHIBuffer> buffer) : m_Buffer(std::move(buffer)) {}

void GfxBuffer::Write(const void *data, uint64 size, uint64 offset) {
	m_Buffer->Write(data, size, offset);
}

void *GfxBuffer::Map() {
	return m_Buffer->Map();
}
void GfxBuffer::Unmap() {
	m_Buffer->Unmap();
}
void GfxBuffer::DestroyImmediate() {
	m_Buffer->DestroyImmediate();
}
void GfxBuffer::Flush(uint64 size, uint64 offset) {
	m_Buffer->Flush(size, offset);
}
uint64 GfxBuffer::GetSize() const {
	return m_Buffer->GetSize();
}
uint32 GfxBuffer::GetInstanceCount() const {
	return m_Buffer->GetInstanceCount();
}
bool GfxBuffer::IsMapped() const {
	return m_Buffer->IsMapped();
}

} // namespace Aquila::GFX
