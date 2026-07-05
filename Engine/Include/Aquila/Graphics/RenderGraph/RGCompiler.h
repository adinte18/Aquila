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
	RHI::ResourceState old_state;
	RHI::ResourceState new_state;
};

struct RGBufBarrier {
	RGBufferHandle handle;
	RHI::ResourceState old_state;
	RHI::ResourceState new_state;
};

struct RGCompiledGraph {
	// Topologically sorted, culled pass indices (into RenderGraph::GetPasses()).
	std::vector<Uint32> pass_order;

	// Flat barrier table for textures.
	// Pass i owns: texBarriers[ passTexBarStart[i] .. passTexBarStart[i+1] )
	std::vector<RGTexBarrier> tex_barriers;
	std::vector<Uint32> pass_tex_bar_start; // size = passOrder.size() + 1

	// Flat barrier table for buffers.
	std::vector<RGBufBarrier> buf_barriers;
	std::vector<Uint32> pass_buf_bar_start; // size = passOrder.size() + 1

	// Per-pass renderpass handle.  Null for compute / copy passes.
	// Ref-counted so the GfxContext's internal caching works correctly.
	std::vector<Ref<GFX::GfxRenderPass>> pass_render_passes;

	// Transient resource owners — alive until Reset() so raw pointers in the
	// registry remain valid throughout Execute().
	std::vector<Ref<GFX::GfxTexture>> transient_textures;
	std::vector<Ref<GFX::GfxBuffer>> transient_buffers;

	bool valid = false;

	void reset() {
		pass_order.clear();
		tex_barriers.clear();
		pass_tex_bar_start.clear();
		buf_barriers.clear();
		pass_buf_bar_start.clear();
		pass_render_passes.clear();
		transient_textures.clear();
		transient_buffers.clear();
		valid = false;
	}
};

class RGCompiler {
  public:
	static RGCompiledGraph compile(const std::vector<RGPassData> &passes, RGRegistry &registry, GFX::GfxContext &ctx);

  private:
	// Directed adjacency list for the dependency graph.
	// adjacency[i] = set of pass indices that depend on pass i.
	using AdjList = std::vector<std::vector<Uint32>>;

	struct LifetimeInterval {
		Int32 first_use = INT32_MAX;
		Int32 last_use = -1;
		bool imported = false;
	};

	// Pool entry for texture aliasing.
	struct TexPoolEntry {
		Ref<GFX::GfxTexture> tex;
		Int32 last_used_at = -1;
		RGTextureDesc desc;
	};

	struct BufPoolEntry {
		Ref<GFX::GfxBuffer> buf;
		Int32 last_used_at = -1;
		RGBufferDesc desc;
	};

	static AdjList build_dependency_graph(const std::vector<RGPassData> &passes, Uint32 tex_count, Uint32 buf_count);

	// Returns false and fills outCyclePath if a cycle is detected.
	static bool topological_sort(const AdjList &adj, Uint32 pass_count, std::vector<Uint32> &out_order,
								std::vector<Uint32> &out_cycle_path);

	static std::vector<bool> cull_passes(const std::vector<RGPassData> &passes, const AdjList &adj,
										const std::vector<Uint32> &sorted_order, const RGRegistry &registry);

	static std::vector<LifetimeInterval> compute_tex_lifetimes(const std::vector<RGPassData> &passes,
															 const std::vector<Uint32> &sorted_order,
															 const std::vector<bool> &alive, Uint32 tex_count,
															 const RGRegistry &registry);

	static std::vector<LifetimeInterval> compute_buf_lifetimes(const std::vector<RGPassData> &passes,
															 const std::vector<Uint32> &sorted_order,
															 const std::vector<bool> &alive, Uint32 buf_count,
															 const RGRegistry &registry);

	static void allocate_transients(const std::vector<RGPassData> &passes, RGRegistry &registry,
								   const std::vector<LifetimeInterval> &tex_lifetimes,
								   const std::vector<LifetimeInterval> &buf_lifetimes, GFX::GfxContext &ctx,
								   RGCompiledGraph &out);

	static void infer_barriers(const std::vector<RGPassData> &passes, const std::vector<Uint32> &sorted_order,
							  const std::vector<bool> &alive, Uint32 tex_count, Uint32 buf_count,
							  const RGRegistry &registry, RGCompiledGraph &out);

	static void create_render_passes(const std::vector<RGPassData> &passes, const std::vector<Uint32> &sorted_order,
								   const std::vector<bool> &alive, const RGRegistry &registry, GFX::GfxContext &ctx,
								   RGCompiledGraph &out);

	static Uint32 slot_of(Uint32 id);
	static bool tex_desc_compatible(const RGTextureDesc &a, const RGTextureDesc &b);
	static bool buf_desc_compatible(const RGBufferDesc &a, const RGBufferDesc &b);
};

} // namespace Aquila::Graphics::RG
