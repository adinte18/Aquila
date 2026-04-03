#ifndef AQUILA_IRHI_DESCRIPTORS_H
#define AQUILA_IRHI_DESCRIPTORS_H

#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::RHI {

// Minimal abstract interface — descriptor systems are heavily API-specific.
// Vulkan provides its full concrete implementation via VulkanDescriptors.h.
// This interface exists as an extension point for future backends.
class IRHIDescriptorSetLayout {
  public:
	virtual ~IRHIDescriptorSetLayout() = default;

	IRHIDescriptorSetLayout(const IRHIDescriptorSetLayout &) = delete;
	IRHIDescriptorSetLayout &operator=(const IRHIDescriptorSetLayout &) = delete;

	[[nodiscard]] virtual uint32 GetBindingCount() const = 0;

  protected:
	IRHIDescriptorSetLayout() = default;
};

class IRHIBuffer;
class IRHITexture;

class IRHIDescriptorSet {
  public:
	virtual ~IRHIDescriptorSet() = default;

	IRHIDescriptorSet(const IRHIDescriptorSet &) = delete;
	IRHIDescriptorSet &operator=(const IRHIDescriptorSet &) = delete;

	virtual void SetBuffer(uint32 binding, IRHIBuffer &buffer, uint64 offset = 0, uint64 range = 0) = 0;
	virtual void SetTexture(uint32 binding, IRHITexture &texture) = 0;
	virtual void Flush() = 0;

  protected:
	IRHIDescriptorSet() = default;
};

} // namespace Aquila::RHI
#endif
