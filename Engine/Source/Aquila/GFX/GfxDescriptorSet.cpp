#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::GFX {

GfxDescriptorSetLayout::GfxDescriptorSetLayout(Unique<RHI::IRHIDescriptorSetLayout> layout)
	: m_layout(std::move(layout)) {}

Uint32 GfxDescriptorSetLayout::get_binding_count() const {
	return m_layout->get_binding_count();
}

GfxDescriptorSet::GfxDescriptorSet(Unique<RHI::IRHIDescriptorSet> set) : m_set(std::move(set)) {}

GfxDescriptorSet &GfxDescriptorSet::set_buffer(Uint32 binding, GfxBuffer &buffer, Uint64 offset, Uint64 range) {
	m_set->set_buffer(binding, buffer.get_rhi(), offset, range);
	return *this;
}

GfxDescriptorSet &GfxDescriptorSet::set_texture(Uint32 binding, GfxTexture &texture) {
	m_set->set_texture(binding, texture.get_rhi());
	return *this;
}

void GfxDescriptorSet::flush() {
	m_set->flush();
}

} // namespace Aquila::GFX
