#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/RenderGraph/RGPassBuilder.h"
#include "Aquila/Graphics/RenderGraph/RGRegistry.h"
#include "Aquila/GFX/GfxRenderpass.h"

namespace Aquila::GFX {
class GfxContext;
class GfxTexture;
class GfxBuffer;
} // namespace Aquila::GFX

namespace Aquila::Graphics::RG {

// Compiled barrier records — stored in SoA flat tables for cache-friendly replay.

struct RGTexBarrier {
	RGTextureHandle handle;
	RHI::ResourceState oldState;
	RHI::ResourceState newState;
};

struct RGBufBarrier {
	RGBufferHandle handle;
	RHI::ResourceState oldState;
	RHI::ResourceState newState;
};

struct RGCompiledGraph {
	// Topologically sorted, culled pass indices (into RenderGraph::GetPasses()).
	std::vector<uint32> passOrder;

	// Flat barrier table for textures.
	// Pass i owns: texBarriers[ passTexBarStart[i] .. passTexBarStart[i+1] )
	std::vector<RGTexBarrier> texBarriers;
	std::vector<uint32> passTexBarStart; // size = passOrder.size() + 1

	// Flat barrier table for buffers.
	std::vector<RGBufBarrier> bufBarriers;
	std::vector<uint32> passBufBarStart; // size = passOrder.size() + 1

	// Per-pass renderpass handle.  Null for compute / copy passes.
	// Ref-counted so the GfxContext's internal caching works correctly.
	std::vector<Ref<GFX::GfxRenderPass>> passRenderPasses;

	// Transient resource owners — alive until Reset() so raw pointers in the
	// registry remain valid throughout Execute().
	std::vector<Ref<GFX::GfxTexture>> transientTextures;
	std::vector<Ref<GFX::GfxBuffer>> transientBuffers;

	bool valid = false;

	void Reset() {
		passOrder.clear();
		texBarriers.clear();
		passTexBarStart.clear();
		bufBarriers.clear();
		passBufBarStart.clear();
		passRenderPasses.clear();
		transientTextures.clear();
		transientBuffers.clear();
		valid = false;
	}
};

class RGCompiler {
  public:
	static RGCompiledGraph Compile(const std::vector<RGPassData> &passes, RGRegistry &registry, GFX::GfxContext &ctx);

  private:
	// Directed adjacency list for the dependency graph.
	// adjacency[i] = set of pass indices that depend on pass i.
	using AdjList = std::vector<std::vector<uint32>>;

	struct LifetimeInterval {
		int32 firstUse = INT32_MAX;
		int32 lastUse = -1;
		bool imported = false;
	};

	// Pool entry for texture aliasing.
	struct TexPoolEntry {
		Ref<GFX::GfxTexture> tex;
		int32 lastUsedAt = -1;
		RGTextureDesc desc;
	};

	struct BufPoolEntry {
		Ref<GFX::GfxBuffer> buf;
		int32 lastUsedAt = -1;
		RGBufferDesc desc;
	};

	static AdjList BuildDependencyGraph(const std::vector<RGPassData> &passes, uint32 texCount, uint32 bufCount);

	// Returns false and fills outCyclePath if a cycle is detected.
	static bool TopologicalSort(const AdjList &adj, uint32 passCount, std::vector<uint32> &outOrder,
								std::vector<uint32> &outCyclePath);

	static std::vector<bool> CullPasses(const std::vector<RGPassData> &passes, const AdjList &adj,
										const std::vector<uint32> &sortedOrder, const RGRegistry &registry);

	static std::vector<LifetimeInterval> ComputeTexLifetimes(const std::vector<RGPassData> &passes,
															 const std::vector<uint32> &sortedOrder,
															 const std::vector<bool> &alive, uint32 texCount,
															 const RGRegistry &registry);

	static std::vector<LifetimeInterval> ComputeBufLifetimes(const std::vector<RGPassData> &passes,
															 const std::vector<uint32> &sortedOrder,
															 const std::vector<bool> &alive, uint32 bufCount,
															 const RGRegistry &registry);

	static void AllocateTransients(const std::vector<RGPassData> &passes, RGRegistry &registry,
								   const std::vector<LifetimeInterval> &texLifetimes,
								   const std::vector<LifetimeInterval> &bufLifetimes, GFX::GfxContext &ctx,
								   RGCompiledGraph &out);

	static void InferBarriers(const std::vector<RGPassData> &passes, const std::vector<uint32> &sortedOrder,
							  const std::vector<bool> &alive, uint32 texCount, uint32 bufCount, RGCompiledGraph &out);

	static void CreateRenderPasses(const std::vector<RGPassData> &passes, const std::vector<uint32> &sortedOrder,
								   const std::vector<bool> &alive, const RGRegistry &registry, GFX::GfxContext &ctx,
								   RGCompiledGraph &out);

	static uint32 SlotOf(uint32 id);
	static bool TexDescCompatible(const RGTextureDesc &a, const RGTextureDesc &b);
	static bool BufDescCompatible(const RGBufferDesc &a, const RGBufferDesc &b);
};

} // namespace Aquila::Graphics::RG
