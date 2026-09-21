#include "Aquila/GFX/GfxMesh.h"
#include "Aquila/GFX/GfxContext.h"

namespace Aquila::GFX {

Ref<GfxMesh> GfxMesh::create(GfxContext &ctx, std::span<const RHI::Vertex> vertices, std::span<const Uint32> indices) {
	auto gfx_mesh = Ref<GfxMesh>(new GfxMesh());
	gfx_mesh->m_index_count = static_cast<Uint32>(indices.size());

	const Uint64 vb_size = vertices.size_bytes();
	const Uint64 ib_size = indices.size_bytes();

	gfx_mesh->m_vertex_buffer = ctx.create_buffer({
		.size = vb_size,
		.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::TransferDst,
		.domain = RHI::MemoryDomain::GpuOnly,
		.debug_name = "GfxMesh_VB",
	});
	gfx_mesh->m_index_buffer = ctx.create_buffer({
		.size = ib_size,
		.usage = RHI::BufferUsage::IndexBuffer | RHI::BufferUsage::TransferDst,
		.domain = RHI::MemoryDomain::GpuOnly,
		.debug_name = "GfxMesh_IB",
	});

	auto vb_stage = ctx.create_buffer({
		.size = vb_size,
		.usage = RHI::BufferUsage::TransferSrc,
		.domain = RHI::MemoryDomain::CpuOnly,
		.debug_name = "GfxMesh_VB_Stage",
	});
	auto ib_stage = ctx.create_buffer({
		.size = ib_size,
		.usage = RHI::BufferUsage::TransferSrc,
		.domain = RHI::MemoryDomain::CpuOnly,
		.debug_name = "GfxMesh_IB_Stage",
	});

	vb_stage->write(vertices.data(), vb_size);
	ib_stage->write(indices.data(), ib_size);

	ctx.copy_buffer(*vb_stage, *gfx_mesh->m_vertex_buffer, vb_size);
	ctx.copy_buffer(*ib_stage, *gfx_mesh->m_index_buffer, ib_size);

	return gfx_mesh;
}

} // namespace Aquila::GFX
