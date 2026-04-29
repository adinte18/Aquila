#include "Aquila/GFX/GfxMesh.h"
#include "Aquila/GFX/GfxContext.h"

namespace Aquila::GFX {

Ref<GfxMesh> GfxMesh::Create(GfxContext &ctx, const Graphics::Resources::Mesh &mesh) {
	auto gfxMesh = Ref<GfxMesh>(new GfxMesh());
	gfxMesh->m_IndexCount = mesh.GetIndexCount();

	const uint64 vbSize = mesh.GetVertexCount() * sizeof(RHI::Vertex);
	const uint64 ibSize = mesh.GetIndexCount() * sizeof(uint32);

	gfxMesh->m_VertexBuffer = ctx.CreateBuffer({
		.size = vbSize,
		.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::TransferDst,
		.domain = RHI::MemoryDomain::GPU_ONLY,
		.debugName = "GfxMesh_VB",
	});
	gfxMesh->m_IndexBuffer = ctx.CreateBuffer({
		.size = ibSize,
		.usage = RHI::BufferUsage::IndexBuffer | RHI::BufferUsage::TransferDst,
		.domain = RHI::MemoryDomain::GPU_ONLY,
		.debugName = "GfxMesh_IB",
	});

	auto vbStage = ctx.CreateBuffer({
		.size = vbSize,
		.usage = RHI::BufferUsage::TransferSrc,
		.domain = RHI::MemoryDomain::CPU_ONLY,
		.debugName = "GfxMesh_VB_Stage",
	});
	auto ibStage = ctx.CreateBuffer({
		.size = ibSize,
		.usage = RHI::BufferUsage::TransferSrc,
		.domain = RHI::MemoryDomain::CPU_ONLY,
		.debugName = "GfxMesh_IB_Stage",
	});

	vbStage->Write(mesh.GetVertices().data(), vbSize);
	ibStage->Write(mesh.GetIndices().data(), ibSize);

	ctx.CopyBuffer(*vbStage, *gfxMesh->m_VertexBuffer, vbSize);
	ctx.CopyBuffer(*ibStage, *gfxMesh->m_IndexBuffer, ibSize);

	return gfxMesh;
}

} // namespace Aquila::GFX
