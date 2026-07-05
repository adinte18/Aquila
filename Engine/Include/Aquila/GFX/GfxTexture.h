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

	void destroy_immediate();
	[[nodiscard]] bool is_ready() const;

	[[nodiscard]] Uint32 get_width() const;
	[[nodiscard]] Uint32 get_height() const;
	[[nodiscard]] Uint32 get_mip_levels() const;
	[[nodiscard]] Uint32 get_array_layers() const;
	[[nodiscard]] RHI::TextureFormat get_format() const;
	[[nodiscard]] const RHI::TextureDesc &get_desc() const;
	[[nodiscard]] RHI::IRHITexture &get_rhi() { return *m_texture; }

  private:
	friend class GfxContext;
	explicit GfxTexture(Unique<RHI::IRHITexture> texture);
	Unique<RHI::IRHITexture> m_texture;
};

} // namespace Aquila::GFX
