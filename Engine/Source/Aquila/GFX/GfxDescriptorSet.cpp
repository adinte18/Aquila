#include "Aquila/GFX/GfxDescriptorSet.h"

namespace Aquila::GFX {

GfxDescriptorSetLayout::GfxDescriptorSetLayout(Unique<RHI::IRHIDescriptorSetLayout> layout)
	: m_Layout(std::move(layout)) {}

uint32 GfxDescriptorSetLayout::GetBindingCount() const {
	return m_Layout->GetBindingCount();
}

GfxDescriptorSet::GfxDescriptorSet(Unique<RHI::IRHIDescriptorSet> set) : m_Set(std::move(set)) {}

GfxDescriptorSet &GfxDescriptorSet::SetBuffer(uint32 binding, GfxBuffer &buffer, uint64 offset, uint64 range) {
	m_Set->SetBuffer(binding, buffer.GetRHI(), offset, range);
	return *this;
}

GfxDescriptorSet &GfxDescriptorSet::SetTexture(uint32 binding, GfxTexture &texture) {
	m_Set->SetTexture(binding, texture.GetRHI());
	return *this;
}

void GfxDescriptorSet::Flush() {
	m_Set->Flush();
}

} // namespace Aquila::GFX
