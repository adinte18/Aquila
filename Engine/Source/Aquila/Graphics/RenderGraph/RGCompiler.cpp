#include "Aquila/Graphics/RenderGraph/RGCompiler.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxRenderpass.h"
#include "Aquila/RHI/Backend/RHITypes.h"

// https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/
namespace Aquila::Graphics::RG {

// bit packing magic
// its packing 2 things: resource id and its version
// 32 bit integer split into 2 :
// [8 bits for version and 24 bits for the slot]
uint32 RGCompiler::SlotOf(uint32 id) {
	return id & 0x00FFFFFFu;
}

// shock, the function does exactly what the name says
// verifies if two texture descriptors are compatible, i.e same shape, format, etc.
bool RGCompiler::TexDescCompatible(const RGTextureDesc &a, const RGTextureDesc &b) {
	return a.width == b.width && a.height == b.height && a.mipLevels == b.mipLevels && a.arrayLayers == b.arrayLayers &&
		a.format == b.format && a.usage == b.usage && a.samples == b.samples;
}

bool RGCompiler::BufDescCompatible(const RGBufferDesc &a, const RGBufferDesc &b) {
	return a.size == b.size && a.usage == b.usage && a.domain == b.domain;
}

// The juicy part
// Basically this function answers: "given all those accesses, which pass must happen before which?"
// It figures out which pqsses depend on whcih, it will basically go through every pass and check who wrote texture X and who reads this texture?

// Algorithm:
//   For each pass X that writes (slot, ver):
//     - Find the pass Y that wrote (slot, ver-1)       -> edge Y to X (WAW chain)
//     - Find all passes Z that read (slot, ver-1)      -> edge Z to X (WAR)
//   For each pass X that reads (slot, ver):
//     - Find the pass Y that wrote (slot, ver)         -> edge Y to X (RAW)

RGCompiler::AdjList RGCompiler::BuildDependencyGraph(const std::vector<RGPassData> &passes, uint32 texCount,
													 uint32 bufCount) {
	const auto nbPasses = static_cast<uint32>(passes.size());
	AdjList adj(nbPasses);

	// (slot, version) -> writer pass index
	std::unordered_map<uint64, uint32> texWriter; // who wrote? one pass per entry
	std::unordered_map<uint64, uint32> bufWriter;

	// (slot, version) -> [reader pass indices]
	std::unordered_map<uint64, std::vector<uint32>> texReaders; // who read? multiple passes per entry
	std::unordered_map<uint64, std::vector<uint32>> bufReaders;

	texWriter.reserve(texCount * 2);
	bufWriter.reserve(bufCount * 2);

	// generate a key that has zero chance of collision
	// its the same packing as the handle, just bigger so slot and version have plenty of room and never overlap
	auto texKey = [](uint32 slot, uint32 ver) -> uint64 { return (static_cast<uint64>(slot) << 32) | ver; };
	auto bufKey = texKey;

	// First pass: index all writers and readers
	for (uint32 passIndex = 0; passIndex < nbPasses; ++passIndex) {
		const RGPassData &pass = passes[passIndex];

		// example :
		// WriterPass writes texture1 v0 -> texWriter[(scene, 0)] = WriterPass => ONLY 1
		// ReaderPass reads texture1 v0 -> texReaders[(scene, 0)] = [WriterPass] => MULTIPLE

		for (const RGTextureAccess &textureWrite : pass.textureWrites) {
			const uint32 slot = SlotOf(textureWrite.handle.id);
			const uint32 ver = (textureWrite.handle.id >> 24) & 0xFFu;
			texWriter[texKey(slot, ver)] = passIndex;
		}
		for (const RGBufferAccess &bufferWrite : pass.bufferWrites) {
			const uint32 slot = SlotOf(bufferWrite.handle.id);
			const uint32 ver = (bufferWrite.handle.id >> 24) & 0xFFu;
			bufWriter[bufKey(slot, ver)] = passIndex;
		}
		for (const RGTextureAccess &textureRead : pass.textureReads) {
			const uint32 slot = SlotOf(textureRead.handle.id);
			const uint32 ver = (textureRead.handle.id >> 24) & 0xFFu;
			texReaders[texKey(slot, ver)].push_back(passIndex);
		}
		for (const RGBufferAccess &bufferRead : pass.bufferReads) {
			const uint32 slot = SlotOf(bufferRead.handle.id);
			const uint32 ver = (bufferRead.handle.id >> 24) & 0xFFu;
			bufReaders[bufKey(slot, ver)].push_back(passIndex);
		}
	}

	auto addEdge = [&](uint32 from, uint32 to) {
		if (from == to) {
			return;
		}
		// Deduplicate edges to keep the graph lean
		auto &deps = adj[from];
		if (std::ranges::find(deps, to) == deps.end()) {
			deps.push_back(to);
		}
	};

	// Second pass: wire edges
	for (uint32 passIndex = 0; passIndex < nbPasses; ++passIndex) {
		const RGPassData &pass = passes[passIndex];

		// RAW: reader depends on its writer
		for (const RGTextureAccess &a : pass.textureReads) {
			const uint32 slot = SlotOf(a.handle.id);
			const uint32 ver = (a.handle.id >> 24) & 0xFFu;
			if (auto it = texWriter.find(texKey(slot, ver)); it != texWriter.end()) {
				addEdge(it->second, passIndex);
			}
		}
		for (const RGBufferAccess &a : pass.bufferReads) {
			const uint32 slot = SlotOf(a.handle.id);
			const uint32 ver = (a.handle.id >> 24) & 0xFFu;
			if (auto it = bufWriter.find(bufKey(slot, ver)); it != bufWriter.end()) {
				addEdge(it->second, passIndex);
			}
		}

		// WAW chain + WAR: a write to ver V means this pass consumed ver V-1
		for (const RGTextureAccess &a : pass.textureWrites) {
			const uint32 slot = SlotOf(a.handle.id);
			const uint32 ver = (a.handle.id >> 24) & 0xFFu;
			if (ver > 0) {
				// WAW: previous writer of (slot, ver-1) must complete before this write
				if (auto it = texWriter.find(texKey(slot, ver - 1)); it != texWriter.end()) {
					addEdge(it->second, passIndex);
				}
				// WAR: all readers of (slot, ver-1) must complete before this write
				if (auto it = texReaders.find(texKey(slot, ver - 1)); it != texReaders.end()) {
					for (uint32 reader : it->second) {
						addEdge(reader, passIndex);
					}
				}
			}
		}
		for (const RGBufferAccess &a : pass.bufferWrites) {
			const uint32 slot = SlotOf(a.handle.id);
			const uint32 ver = (a.handle.id >> 24) & 0xFFu;
			if (ver > 0) {
				if (auto it = bufWriter.find(bufKey(slot, ver - 1)); it != bufWriter.end()) {
					addEdge(it->second, passIndex);
				}
				if (auto it = bufReaders.find(bufKey(slot, ver - 1)); it != bufReaders.end()) {
					for (uint32 reader : it->second) {
						addEdge(reader, passIndex);
					}
				}
			}
		}
	}

	return adj;
}

// Topological Sort
//
// Stability guarantee: ties broken by original pass index, ensuring the sorted
// order is deterministic across frames and platforms.
//
// Returns false if a cycle is found.  outCyclePath contains the cycle nodes.
// https://www.geeksforgeeks.org/dsa/topological-sorting-indegree-based-solution/
bool RGCompiler::TopologicalSort(const AdjList &adj, uint32 passCount, std::vector<uint32> &outOrder,
								 std::vector<uint32> &outCyclePath) {
	outOrder.clear();
	outOrder.reserve(passCount);

	std::vector<int32> inDegree(passCount, 0);
	for (uint32 i = 0; i < passCount; ++i) {
		for (uint32 dep : adj[i]) {
			++inDegree[dep];
		}
	}

	// Use a min-heap keyed on pass index for stability
	std::priority_queue<uint32, std::vector<uint32>, std::greater<>> ready;
	for (uint32 i = 0; i < passCount; ++i) {
		if (inDegree[i] == 0) {
			ready.push(i);
		}
	}

	while (!ready.empty()) {
		uint32 cur = ready.top();
		ready.pop();
		outOrder.push_back(cur);
		for (uint32 succ : adj[cur]) {
			if (--inDegree[succ] == 0) {
				ready.push(succ);
			}
		}
	}

	if (outOrder.size() < passCount) {
		// gather nodes with remaining in-degree > 0 as the cycle evidence
		for (uint32 i = 0; i < passCount; ++i) {
			if (inDegree[i] > 0) {
				outCyclePath.push_back(i);
			}
		}
		return false;
	}
	return true;
}

// Pass Culling
//
// This will remove any pass that is not contributing to anything visible
// It works backwards from the final outputs (basically smth like what do we actually need on screen?)
// and marks everything that feeds into those.
//
// Any pass that only writes into a texture nobody ever reads gets thrown away. therefore => GPU happy no extra work
//
// A pass is "live" if it:
//  a) writes to an imported resource,
//  OR
//  b) is reachable from a live pass through the *reverse* dependency graph.
//
// Passes that only produce transient resources consumed by dead passes are removed.

std::vector<bool> RGCompiler::CullPasses(const std::vector<RGPassData> &passes, const AdjList &adj,
										 const std::vector<uint32> &sortedOrder, const RGRegistry &registry) {
	const uint32 n = static_cast<uint32>(passes.size());
	std::vector<bool> alive(n, false);

	// Build reverse adjacency list
	AdjList radj(n);
	for (uint32 i = 0; i < n; ++i) {
		for (uint32 succ : adj[i]) {
			radj[succ].push_back(i);
		}
	}

	// Passes with an external side effect (e.g. swapchain blit) are always sinks.
	// Passes that write to an imported resource are also always sinks.
	// Passes with an unsatisfied read dependency are never sinks and can never be made alive.
	for (uint32 pi = 0; pi < n; ++pi) {
		const RGPassData &p = passes[pi];
		if (p.hasUnsatisfiedDep) {
			continue;
		}
		bool isSink = p.hasSideEffect;
		if (!isSink) {
			for (const RGTextureAccess &a : p.textureWrites) {
				if (registry.IsImportedTexture(a.handle)) {
					isSink = true;
					break;
				}
			}
		}
		if (!isSink) {
			for (const RGBufferAccess &a : p.bufferWrites) {
				if (registry.IsImportedBuffer(a.handle)) {
					isSink = true;
					break;
				}
			}
		}
		if (isSink) {
			alive[pi] = true;
		}
	}

	// BFS backwards from sinks
	std::queue<uint32> work;
	for (uint32 i = 0; i < n; ++i) {
		if (alive[i]) {
			work.push(i);
		}
	}

	while (!work.empty()) {
		uint32 cur = work.front();
		work.pop();
		for (uint32 pred : radj[cur]) {
			if (!alive[pred] && !passes[pred].hasUnsatisfiedDep) {
				alive[pred] = true;
				work.push(pred);
			}
		}
	}

	return alive;
}

//  Lifetime analysis
std::vector<RGCompiler::LifetimeInterval> RGCompiler::ComputeTexLifetimes(const std::vector<RGPassData> &passes,
																		  const std::vector<uint32> &sortedOrder,
																		  const std::vector<bool> &alive,
																		  uint32 texCount, const RGRegistry &registry) {
	std::vector<LifetimeInterval> lifetimes(texCount);
	// Use GetTextureVersion to build a current handle for validation-safe lookups
	for (uint32 slot = 0; slot < texCount; ++slot) {
		const uint32 ver = registry.GetTextureVersion(RGTextureHandle{ slot });

		// very confusing i know
		// but look, here is some explanation for  you
		// basic example : ver = 2 and slot = 5
		// 32 bits integer => 00000000 000000000000000000000000 => as explained before 8 bits for the version, 24 bits for the slot
		// ver << 24  = 00000010 000000000000000000000000
		// slot = 00000000 000000000000000000000101
		// OR operation => 00000010 000000000000000000000101
		//
		// this gives us the packed handle ID, that has both pieces of info packed inside version and slot :D
		const uint32 curId = (ver << 24u) | (slot);

		lifetimes[slot].imported = registry.IsImportedTexture(RGTextureHandle{ curId });
	}

	for (int32 pos = 0; pos < static_cast<int32>(sortedOrder.size()); ++pos) {
		const uint32 passIndex = sortedOrder[pos];
		if (!alive[passIndex]) {
			continue;
		}
		const RGPassData &pass = passes[passIndex];

		// tell the texture its used at position X
		auto touch = [&](RGTextureHandle handle) {
			uint32 slot = SlotOf(handle.id);
			lifetimes[slot].firstUse = std::min(lifetimes[slot].firstUse, pos);
			lifetimes[slot].lastUse = std::max(lifetimes[slot].lastUse, pos);
		};

		for (const auto &textureRead : pass.textureReads) {
			touch(textureRead.handle);
		}
		for (const auto &textureWrite : pass.textureWrites) {
			touch(textureWrite.handle);
		}
		for (const auto &colorAttachment : pass.colorAttachments) {
			if (colorAttachment.handle.IsValid()) {
				touch(colorAttachment.handle);
			}
		}
		if (pass.hasDepthAttachment) {
			touch(pass.depthAttachment.handle);
		}
	}
	return lifetimes;
}

std::vector<RGCompiler::LifetimeInterval> RGCompiler::ComputeBufLifetimes(const std::vector<RGPassData> &passes,
																		  const std::vector<uint32> &sortedOrder,
																		  const std::vector<bool> &alive,
																		  uint32 bufCount, const RGRegistry &registry) {
	// ! The usage is the same as for the textures so if you are lost, read the comments on the function above

	std::vector<LifetimeInterval> lifetimes(bufCount);
	for (uint32 slot = 0; slot < bufCount; ++slot) {
		const uint32 ver = registry.GetBufferVersion(RGBufferHandle{ slot });
		const uint32 curId = (ver << 24u) | (slot);
		lifetimes[slot].imported = registry.IsImportedBuffer(RGBufferHandle{ curId });
	}

	for (int32 pos = 0; pos < static_cast<int32>(sortedOrder.size()); ++pos) {
		const uint32 passIndex = sortedOrder[pos];
		if (!alive[passIndex]) {
			continue;
		}
		const RGPassData &pass = passes[passIndex];

		auto touch = [&](RGBufferHandle h) {
			uint32 slot = SlotOf(h.id);
			lifetimes[slot].firstUse = std::min(lifetimes[slot].firstUse, pos);
			lifetimes[slot].lastUse = std::max(lifetimes[slot].lastUse, pos);
		};
		for (const auto &bufferRead : pass.bufferReads) {
			touch(bufferRead.handle);
		}
		for (const auto &bufferWrite : pass.bufferWrites) {
			touch(bufferWrite.handle);
		}
	}
	return lifetimes;
}

// Transient resource allocation with memory aliasing
//
// Greedy interval-coloring: resources with non-overlapping live intervals and
// compatible descriptors share the same physical allocation.
//
// Sort slots by firstUse, then for each slot try to find a free physical
// resource from the pool whose lastUsedAt < slot.firstUse and whose descriptor
// is compatible.  This achieves optimal aliasing for intervals sorted by start.

void RGCompiler::AllocateTransients(const std::vector<RGPassData> &passes, RGRegistry &registry,
									const std::vector<LifetimeInterval> &texLifetimes,
									const std::vector<LifetimeInterval> &bufLifetimes, GFX::GfxContext &ctx,
									RGCompiledGraph &out) {
	const uint32 texCount = registry.TextureCount();

	// build a sorted list of what needs allocating
	std::vector<std::pair<int32, uint32>> texOrder;
	texOrder.reserve(texCount);
	for (uint32 slot = 0; slot < texCount; ++slot) {
		const LifetimeInterval &lt = texLifetimes[slot];
		// skip if its externally owned or never used
		if (lt.imported || lt.firstUse == INT32_MAX) {
			continue; // never used or external
		}
		texOrder.emplace_back(lt.firstUse, slot);
	}
	// sort by first use (super hack to make greedy recycling work optimally)
	std::ranges::stable_sort(texOrder);

	std::vector<TexPoolEntry> texPool;
	texPool.reserve(texOrder.size());

	// try to recylce whatever is recyclable, otherwise just allocate fresh
	for (auto [firstUse, slot] : texOrder) {
		const uint32 tver = registry.GetTextureVersion(RGTextureHandle{ slot });
		const uint32 tId = (tver << 24u) | (slot);
		const RGTextureDesc &desc = registry.GetTextureDesc(RGTextureHandle{ tId });
		const int32 last = texLifetimes[slot].lastUse;

		// Try to find a free compatible physical texture
		TexPoolEntry *match = nullptr;
		// foreach entry in the current texture pool
		for (TexPoolEntry &entry : texPool) {
			// is the texture free and is it compatible?
			if (entry.lastUsedAt < firstUse && TexDescCompatible(entry.desc, desc)) {
				if ((match == nullptr) || entry.lastUsedAt > match->lastUsedAt) {
					match = &entry; // Prefer most-recently-freed for better cache locality
				}
			}
		}

		// if we have a match => recycle it
		if (match != nullptr) {
			match->lastUsedAt = last;
			registry.ResolveTexture(RGTextureHandle{ tId }, match->tex.get());
		} else {
			// if no match => allocate a new physical texture.
			// Convert RGTextureDesc -> RHI::TextureDesc.
			RHI::TextureDesc rhiDesc{};
			rhiDesc.width = desc.width;
			rhiDesc.height = desc.height;
			rhiDesc.mipLevels = desc.mipLevels;
			rhiDesc.arrayLayers = desc.arrayLayers;
			rhiDesc.format = desc.format;
			rhiDesc.usage = desc.usage;
			rhiDesc.samples = desc.samples;
			rhiDesc.debugName = desc.debugName.empty() ? "RG_Transient" : std::string(desc.debugName);

			// viewType: derive from arrayLayers and format
			if (rhiDesc.arrayLayers == 6) {
				rhiDesc.viewType = RHI::TextureViewType::Cube;
			} else if (rhiDesc.arrayLayers > 1) {
				rhiDesc.viewType = RHI::TextureViewType::Tex2DArray;
			} else {
				rhiDesc.viewType = RHI::TextureViewType::Tex2D;
			}

			Ref<GFX::GfxTexture> newTex = ctx.CreateTexture(rhiDesc);
			AQUILA_ASSERT(newTex, "Failed to allocate transient texture");

			out.transientTextures.push_back(newTex);
			registry.ResolveTexture(RGTextureHandle{ tId }, newTex.get());

			texPool.push_back(TexPoolEntry{ .tex = newTex, .lastUsedAt = last, .desc = desc });
		}
	}

	// do the same for buffers
	const uint32 bufCount = registry.BufferCount();

	std::vector<std::pair<int32, uint32>> bufOrder;
	bufOrder.reserve(bufCount);

	for (uint32 slot = 0; slot < bufCount; ++slot) {
		const LifetimeInterval &lt = bufLifetimes[slot];
		if (lt.imported || lt.firstUse == INT32_MAX) {
			continue;
		}
		bufOrder.emplace_back(lt.firstUse, slot);
	}
	std::ranges::stable_sort(bufOrder);

	std::vector<BufPoolEntry> bufPool;
	bufPool.reserve(bufOrder.size());

	for (auto [firstUse, slot] : bufOrder) {
		const uint32 bver = registry.GetBufferVersion(RGBufferHandle{ slot });
		const uint32 bId = (bver << 24u) | (slot);
		const RGBufferDesc &desc = registry.GetBufferDesc(RGBufferHandle{ bId });
		const int32 last = bufLifetimes[slot].lastUse;

		BufPoolEntry *match = nullptr;
		for (BufPoolEntry &entry : bufPool) {
			if (entry.lastUsedAt < firstUse && BufDescCompatible(entry.desc, desc)) {
				if ((match == nullptr) || entry.lastUsedAt > match->lastUsedAt) {
					match = &entry;
				}
			}
		}

		if (match != nullptr) {
			match->lastUsedAt = last;
			registry.ResolveBuffer(RGBufferHandle{ bId }, match->buf.get());
		} else {
			RHI::BufferDesc rhiDesc{};
			rhiDesc.size = desc.size;
			rhiDesc.usage = desc.usage;
			rhiDesc.domain = desc.domain;
			rhiDesc.debugName = desc.debugName.empty() ? "RG_Transient" : std::string(desc.debugName);

			Ref<GFX::GfxBuffer> newBuf = ctx.CreateBuffer(rhiDesc);
			AQUILA_ASSERT(newBuf, "Failed to allocate transient buffer");

			out.transientBuffers.push_back(newBuf);
			registry.ResolveBuffer(RGBufferHandle{ bId }, newBuf.get());

			bufPool.push_back(BufPoolEntry{ .buf = newBuf, .lastUsedAt = last, .desc = desc });
		}
	}
}

// Barrier inference
// Walk the sorted, culled pass list.  Per-resource state is tracked in flat
// arrays indexed by slot.  Whenever required state != current state, push a
// barrier record into the flat table and update current state.

void RGCompiler::InferBarriers(const std::vector<RGPassData> &passes, const std::vector<uint32> &sortedOrder,
							   const std::vector<bool> &alive, uint32 texCount, uint32 bufCount,
							   const RGRegistry &registry, RGCompiledGraph &out) {
	// Imported resources start from their declared initial state, not Undefined,
	// so persistent textures don't get their contents discarded on first use.
	std::vector<RHI::ResourceState> curTexState(texCount, RHI::ResourceState::Undefined);
	for (uint32 slot = 0; slot < texCount; ++slot) {
		if (registry.IsImportedTexture(RGTextureHandle{ slot })) {
			curTexState[slot] = registry.GetTextureInitialState(RGTextureHandle{ slot });
		}
	}

	std::vector<RHI::ResourceState> curBufState(bufCount, RHI::ResourceState::Undefined);
	for (uint32 slot = 0; slot < bufCount; ++slot) {
		if (registry.IsImportedBuffer(RGBufferHandle{ slot })) {
			curBufState[slot] = registry.GetBufferInitialState(RGBufferHandle{ slot });
		}
	}

	const uint32 aliveCount = static_cast<uint32>(std::count(alive.begin(), alive.end(), true));

	// texBarriers is a flat list of ALL barriers jammed together
	out.texBarriers.reserve(aliveCount * 2);
	out.bufBarriers.reserve(aliveCount);

	// to keep track and to know which barriers belong to which pass
	// for example : for pass N, we have M barriers
	//  texBarriers[ passTexBarStart[N] ... passTexBarStart[N+1] ]
	out.passTexBarStart.reserve(aliveCount + 1); // +1 for the sentinel
	out.passBufBarStart.reserve(aliveCount + 1); // same as before

	for (const uint32 passIndex : sortedOrder) {
		if (!alive[passIndex]) {
			continue;
		}
		const RGPassData &pass = passes[passIndex];

		// so if my texBarriers at this point is of size N, then the index table passTexBarStart begins at index N in the texBarriers vector
		// when the pass runs and pushes its barriers into texBarriers,
		// they naturally land at index N, N+1, etc.
		// until the next pass comes along and starts recording its own start position
		out.passTexBarStart.push_back(static_cast<uint32>(out.texBarriers.size()));
		out.passBufBarStart.push_back(static_cast<uint32>(out.bufBarriers.size()));

		// Helper: emit a texture barrier if state changed
		auto maybeTextureBarrier = [&](RGTextureHandle handle, RHI::ResourceState required) {
			const uint32 slot = SlotOf(handle.id);

			// if textures current state is not what the next pass needs
			if (curTexState[slot] != required) {
				// record a barrier from current state to required state
				out.texBarriers.push_back({ handle, curTexState[slot], required });

				// update current state
				curTexState[slot] = required;
			}
		};

		// the same as before just for buffers
		auto maybeBufferBarrier = [&](RGBufferHandle handle, RHI::ResourceState required) {
			const uint32 slot = SlotOf(handle.id);
			if (curBufState[slot] != required) {
				out.bufBarriers.push_back({ handle, curBufState[slot], required });
				curBufState[slot] = required;
			}
		};

		for (const RGTextureAccess &textureRead : pass.textureReads) {
			maybeTextureBarrier(textureRead.handle, textureRead.state);
		}
		for (const RGTextureAccess &textureWrite : pass.textureWrites) {
			maybeTextureBarrier(textureWrite.handle, textureWrite.state);
		}
		for (const RGBufferAccess &bufferRead : pass.bufferReads) {
			maybeBufferBarrier(bufferRead.handle, bufferRead.state);
		}
		for (const RGBufferAccess &bufferWrite : pass.bufferWrites) {
			maybeBufferBarrier(bufferWrite.handle, bufferWrite.state);
		}
	}

	// a dummy end marker so that we can access barriers for the very last pass
	//
	// without dummy
	//  texBarriers     = [B0, B1, B2, B3, B4]
	//  passTexBarStart = [0, 2, 4]
	// with dummy
	//  passTexBarStart = [0, 2, 4, 5]

	out.passTexBarStart.push_back(static_cast<uint32>(out.texBarriers.size()));
	out.passBufBarStart.push_back(static_cast<uint32>(out.bufBarriers.size()));
}

void RGCompiler::CreateRenderPasses(const std::vector<RGPassData> &passes, const std::vector<uint32> &sortedOrder,
									const std::vector<bool> &alive, const RGRegistry &registry, GFX::GfxContext &ctx,
									RGCompiledGraph &out) {
	// passRenderPasses is indexed by position in passOrder (alive passes only)
	out.passRenderPasses.resize(out.passOrder.size());

	uint32 schedPos = 0;
	for (const uint32 passIndex : sortedOrder) {
		if (!alive[passIndex]) {
			continue;
		}
		const RGPassData &pass = passes[passIndex];

		const bool hasColor = !pass.colorAttachments.empty();
		const bool hasDepth = pass.hasDepthAttachment;

		if (!hasColor && !hasDepth) {
			++schedPos;
			continue;
		} // Compute / copy pass

		RHI::RenderPassDesc rpDesc{};
		rpDesc.debugName = pass.name;
		rpDesc.externalBarriers = true; // Graph owns all barriers

		// Color attachments
		rpDesc.colorAttachments.reserve(pass.colorAttachments.size());
		for (const RGColorAttachment &rga : pass.colorAttachments) {
			if (!rga.handle.IsValid()) {
				continue;
			}

			GFX::GfxTexture &tex = registry.GetTexture(rga.handle);

			RHI::RenderPassColorAttachmentDesc colorAttachment{};
			colorAttachment.texture = &tex.GetRHI();
			colorAttachment.clearColor = { rga.clear.color.r, rga.clear.color.g, rga.clear.color.b, rga.clear.color.a };
			colorAttachment.loadOp = static_cast<RHI::AttachmentLoadOp>(rga.loadOp);
			colorAttachment.storeOp = static_cast<RHI::AttachmentStoreOp>(rga.storeOp);

			rpDesc.colorAttachments.push_back(colorAttachment);
		}

		// Depth attachment
		if (hasDepth) {
			const RGDepthAttachment &rda = pass.depthAttachment;
			GFX::GfxTexture &tex = registry.GetTexture(rda.handle);

			RHI::RenderPassDepthAttachmentDesc depthAttachment{};
			depthAttachment.texture = &tex.GetRHI();
			depthAttachment.depthLoadOp = static_cast<RHI::AttachmentLoadOp>(rda.depthLoadOp);
			depthAttachment.depthStoreOp = static_cast<RHI::AttachmentStoreOp>(rda.depthStoreOp);
			depthAttachment.stencilLoadOp = static_cast<RHI::AttachmentLoadOp>(rda.stencilLoadOp);
			depthAttachment.stencilStoreOp = static_cast<RHI::AttachmentStoreOp>(rda.stencilStoreOp);
			depthAttachment.readOnly = rda.readOnly;
			depthAttachment.clearDepth = rda.clear.depth;
			depthAttachment.clearStencil = rda.clear.stencil;

			rpDesc.depthAttachment = depthAttachment;
		}

		// Derive dimensions from the first valid attachment
		if (!rpDesc.colorAttachments.empty() && (rpDesc.colorAttachments[0].texture != nullptr)) {
			rpDesc.width = rpDesc.colorAttachments[0].texture->GetWidth();
			rpDesc.height = rpDesc.colorAttachments[0].texture->GetHeight();
		} else if (hasDepth && rpDesc.depthAttachment) {
			rpDesc.width = rpDesc.depthAttachment->texture->GetWidth();
			rpDesc.height = rpDesc.depthAttachment->texture->GetHeight();
		}

		out.passRenderPasses[schedPos] = ctx.CreateRenderPass(rpDesc);
		++schedPos;
	}
}

// Public entry point
RGCompiledGraph RGCompiler::Compile(const std::vector<RGPassData> &passes, RGRegistry &registry, GFX::GfxContext &ctx) {
	RGCompiledGraph out;

	if (passes.empty()) {
		out.valid = true;
		return out;
	}

	const auto passCount = static_cast<uint32>(passes.size());
	const uint32 texCount = registry.TextureCount();
	const uint32 bufCount = registry.BufferCount();

	AdjList adj = BuildDependencyGraph(passes, texCount, bufCount);

	std::vector<uint32> sortedOrder;
	std::vector<uint32> cyclePath;
	const bool acyclic = TopologicalSort(adj, passCount, sortedOrder, cyclePath);

	if (!acyclic) {
		AQUILA_LOG_CRITICAL("Rendergraph cycle detected involving passes : ");
		for (uint32 passIndex : cyclePath) {
			AQUILA_LOG_CRITICAL("   {}", passes[passIndex].name);
		}
		return out;
	}

	std::vector<bool> alive = CullPasses(passes, adj, sortedOrder, registry);

	// Build final live-only pass order
	out.passOrder.reserve(passCount);
	for (uint32 passIndex : sortedOrder) {
		if (alive[passIndex]) {
			out.passOrder.push_back(passIndex);
		}
	}

	std::vector<LifetimeInterval> texLifetimes = ComputeTexLifetimes(passes, sortedOrder, alive, texCount, registry);
	std::vector<LifetimeInterval> bufLifetimes = ComputeBufLifetimes(passes, sortedOrder, alive, bufCount, registry);

	AllocateTransients(passes, registry, texLifetimes, bufLifetimes, ctx, out);

	InferBarriers(passes, sortedOrder, alive, texCount, bufCount, registry, out);

	CreateRenderPasses(passes, sortedOrder, alive, registry, ctx, out);

	out.valid = true;
	return out;
}

} // namespace Aquila::Graphics::RG
