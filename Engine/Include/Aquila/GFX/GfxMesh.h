#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/Graphics/Resources/Mesh.h"

namespace Aquila::GFX {

class GfxContext;

class GfxMesh {
  public:
	static Ref<GfxMesh> Create(GfxContext &ctx, const Graphics::Resources::Mesh &mesh);

	AQUILA_NONCOPYABLE(GfxMesh);
	AQUILA_NONMOVEABLE(GfxMesh);

	[[nodiscard]] GfxBuffer &GetVertexBuffer() const { return *m_VertexBuffer; }
	[[nodiscard]] GfxBuffer &GetIndexBuffer() const { return *m_IndexBuffer; }
	[[nodiscard]] uint32 GetIndexCount() const { return m_IndexCount; }

  private:
	GfxMesh() = default;
	Ref<GfxBuffer> m_VertexBuffer;
	Ref<GfxBuffer> m_IndexBuffer;
	uint32 m_IndexCount = 0;
};

} // namespace Aquila::GFX
