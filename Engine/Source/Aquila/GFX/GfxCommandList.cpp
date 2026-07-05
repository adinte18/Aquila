#include "Aquila/GFX/GfxCommandList.h"

namespace Aquila::GFX {

GfxCommandList::GfxCommandList(Unique<RHI::IRHICommandList> cmd) : m_cmd(std::move(cmd)) {}

void GfxCommandList::begin() {
	m_cmd->begin();
}
void GfxCommandList::end() {
	m_cmd->end();
}
void GfxCommandList::reset() {
	m_cmd->reset();
}

bool GfxCommandList::is_recording() const {
	return m_cmd->is_recording();
}
RHI::CommandListType GfxCommandList::get_type() const {
	return m_cmd->get_type();
}
const std::string &GfxCommandList::get_name() const {
	return m_cmd->get_name();
}

void GfxCommandList::transition_texture(GfxTexture &texture, RHI::ResourceState old_state, RHI::ResourceState new_state) {
	m_cmd->transition_texture(texture.get_rhi(), old_state, new_state);
}

void GfxCommandList::transition_buffer(GfxBuffer &buffer, RHI::ResourceState old_state, RHI::ResourceState new_state) {
	m_cmd->transition_buffer(buffer.get_rhi(), old_state, new_state);
}

void GfxCommandList::bind_pipeline(GfxPipeline &pipeline) {
	m_cmd->bind_pipeline(pipeline.get_rhi());
}

void GfxCommandList::set_viewport(float x, float y, float width, float height, float min_depth, float max_depth) {
	m_cmd->set_viewport(x, y, width, height, min_depth, max_depth);
}

void GfxCommandList::set_scissor(Int32 x, Int32 y, Uint32 width, Uint32 height) {
	m_cmd->set_scissor(x, y, width, height);
}

void GfxCommandList::bind_descriptor_set(Uint32 set, GfxDescriptorSet &descriptor_set) {
	m_cmd->bind_descriptor_set(set, descriptor_set.get_rhi());
}

void GfxCommandList::bind_vertex_buffer(GfxBuffer &buf, Uint32 binding, Uint64 offset) {
	m_cmd->bind_vertex_buffer(buf.get_rhi(), binding, offset);
}

void GfxCommandList::bind_index_buffer(GfxBuffer &buf, RHI::IndexFormat fmt, Uint64 offset) {
	m_cmd->bind_index_buffer(buf.get_rhi(), fmt, offset);
}

void GfxCommandList::draw(Uint32 vertex_count, Uint32 instance_count, Uint32 first_vertex, Uint32 first_instance) {
	m_cmd->draw(vertex_count, instance_count, first_vertex, first_instance);
}

void GfxCommandList::draw_indexed(Uint32 index_count, Uint32 instance_count, Uint32 first_index, Int32 vertex_offset,
								 Uint32 first_instance) {
	m_cmd->draw_indexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

void GfxCommandList::draw_indirect(GfxBuffer &buffer, Uint64 offset, Uint32 draw_count, Uint32 stride) {
	m_cmd->draw_indirect(buffer.get_rhi(), offset, draw_count, stride);
}

void GfxCommandList::draw_indexed_indirect(GfxBuffer &buffer, Uint64 offset, Uint32 draw_count, Uint32 stride) {
	m_cmd->draw_indexed_indirect(buffer.get_rhi(), offset, draw_count, stride);
}

void GfxCommandList::copy_buffer_to_texture(GfxBuffer &src, GfxTexture &dst, Uint32 width, Uint32 height,
										 Uint32 dst_array_layer, Uint32 dst_mip_level) {
	m_cmd->copy_buffer_to_texture(src.get_rhi(), dst.get_rhi(), width, height, dst_array_layer, dst_mip_level);
}

void GfxCommandList::fill_buffer(GfxBuffer &buffer, Uint32 value, Uint64 offset, Uint64 size) {
	m_cmd->fill_buffer(buffer.get_rhi(), offset, size, value);
}

void GfxCommandList::dispatch(Uint32 x, Uint32 y, Uint32 z) {
	m_cmd->dispatch(x, y, z);
}

void GfxCommandList::push_debug_group(const char *name) {
	m_cmd->push_debug_group(name);
}
void GfxCommandList::pop_debug_group() {
	m_cmd->pop_debug_group();
}

} // namespace Aquila::GFX
