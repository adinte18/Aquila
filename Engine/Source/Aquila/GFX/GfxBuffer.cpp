#include "Aquila/GFX/GfxBuffer.h"

namespace Aquila::GFX {

GfxBuffer::GfxBuffer(Unique<RHI::IRHIBuffer> buffer) : m_buffer(std::move(buffer)) {}

void GfxBuffer::write(const void *data, Uint64 size, Uint64 offset) {
	m_buffer->write(data, size, offset);
}

void *GfxBuffer::map() {
	return m_buffer->map();
}
void GfxBuffer::unmap() {
	m_buffer->unmap();
}
void GfxBuffer::destroy_immediate() {
	m_buffer->destroy_immediate();
}
void GfxBuffer::flush(Uint64 size, Uint64 offset) {
	m_buffer->flush(size, offset);
}
Uint64 GfxBuffer::get_size() const {
	return m_buffer->get_size();
}
Uint32 GfxBuffer::get_instance_count() const {
	return m_buffer->get_instance_count();
}
bool GfxBuffer::is_mapped() const {
	return m_buffer->is_mapped();
}

} // namespace Aquila::GFX
