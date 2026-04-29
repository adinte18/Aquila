#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHIDescriptors.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::GFX {


class GfxContext;
class GfxCommandList;

class GfxDescriptorSetLayout {
  public:
	~GfxDescriptorSetLayout() = default;
	AQUILA_NONCOPYABLE(GfxDescriptorSetLayout);

	[[nodiscard]] uint32 GetBindingCount() const;
	[[nodiscard]] RHI::IRHIDescriptorSetLayout &GetRHI() { return *m_Layout; }

  private:
	friend class GfxContext;
	explicit GfxDescriptorSetLayout(Unique<RHI::IRHIDescriptorSetLayout> layout);
	Unique<RHI::IRHIDescriptorSetLayout> m_Layout;
};

class GfxDescriptorSet {
  public:
	~GfxDescriptorSet() = default;
	AQUILA_NONCOPYABLE(GfxDescriptorSet);

	GfxDescriptorSet &SetBuffer(uint32 binding, GfxBuffer &buffer, uint64 offset = 0, uint64 range = 0);
	GfxDescriptorSet &SetTexture(uint32 binding, GfxTexture &texture);
	void Flush();

	[[nodiscard]] RHI::IRHIDescriptorSet &GetRHI() { return *m_Set; }

  private:
	friend class GfxContext;
	explicit GfxDescriptorSet(Unique<RHI::IRHIDescriptorSet> set);
	Unique<RHI::IRHIDescriptorSet> m_Set;
};

} // namespace Aquila::GFX
