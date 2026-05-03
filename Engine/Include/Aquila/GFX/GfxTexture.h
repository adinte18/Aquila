#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/RHI/Backend/IRHITexture.h"

namespace Aquila::GFX {

class GfxContext;

class GfxTexture {
  public:
	~GfxTexture() = default;
	AQUILA_NONCOPYABLE(GfxTexture);

	void DestroyImmediate();

	[[nodiscard]] uint32 GetWidth() const;
	[[nodiscard]] uint32 GetHeight() const;
	[[nodiscard]] uint32 GetMipLevels() const;
	[[nodiscard]] uint32 GetArrayLayers() const;
	[[nodiscard]] RHI::TextureFormat GetFormat() const;
	[[nodiscard]] const RHI::TextureDesc &GetDesc() const;
	[[nodiscard]] RHI::IRHITexture &GetRHI() { return *m_Texture; }

  private:
	friend class GfxContext;
	explicit GfxTexture(Unique<RHI::IRHITexture> texture);
	Unique<RHI::IRHITexture> m_Texture;
};

} // namespace Aquila::GFX
