#pragma once
#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxMesh.h"
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Rendering/Systems/Base/IRenderingSystem.h"
#include "Aquila/Rendering/FrameContext.h"

namespace Aquila::Rendering {

// RenderingSystemBase
//
// Convenient base for concrete rendering systems. Adds:
//   - m_Ctx pointer set during OnInit so subclasses don't hold it separately.
//   - GetOrUploadMesh() — uploads a CPU Mesh to the GPU on first call and caches the
//     result by pointer, avoiding redundant uploads across frames and systems.
//
// Subclasses must implement AddPasses(). OnInit / OnShutdown are open for extension
// (call the base version when you do).
class RenderingSystemBase : public IRenderingSystem {
  public:
	RenderingSystemBase() = default;
	~RenderingSystemBase() override = default;

	AQUILA_NONCOPYABLE(RenderingSystemBase);
	AQUILA_NONMOVEABLE(RenderingSystemBase);

	void OnInit(GFX::GfxContext &ctx) override { m_Ctx = &ctx; }

	void OnShutdown() override { m_MeshCache.clear(); }

  protected:
	Ref<GFX::GfxMesh> GetOrUploadMesh(const Ref<Graphics::Resources::Mesh> &mesh) {
		auto it = m_MeshCache.find(mesh.get());
		if (it != m_MeshCache.end()) {
			return it->second;
		}
		auto gpu = GFX::GfxMesh::Create(*m_Ctx, *mesh);
		m_MeshCache[mesh.get()] = gpu;
		return gpu;
	}

	GFX::GfxContext *m_Ctx = nullptr;

  private:
	std::unordered_map<Graphics::Resources::Mesh *, Ref<GFX::GfxMesh>> m_MeshCache;
};

} // namespace Aquila::Rendering
