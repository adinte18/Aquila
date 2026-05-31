#include "Aquila/Graphics/Resources/Mesh.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

namespace Aquila::Graphics::Resources {

Mesh::Mesh(const std::string &debugName) : m_DebugName(debugName) {}

void Mesh::Load(const std::string &filepath) {
	Assimp::Importer importer;

	const auto file =
		Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, AccessMode::Read, OpenMode::Binary);
	if (!file || !file->IsValid()) {
		throw std::runtime_error("Failed to open mesh via VFS: " + filepath);
	}

	const int64 fileSize = file->Size();
	if (fileSize <= 0) {
		throw std::runtime_error("Mesh file is empty: " + filepath);
	}

	std::vector<uint8> buffer(fileSize);
	if (const size_t bytesRead = file->Read(buffer.data(), buffer.size()); bytesRead != buffer.size()) {
		throw std::runtime_error("Failed to read entire mesh buffer from VFS");
	}

	uint32 flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality;
	if (fileSize < 5 * 1024 * 1024) {
		flags |= aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_GenUVCoords;
	}

	const aiScene *scene = importer.ReadFileFromMemory(buffer.data(), buffer.size(), flags);
	if ((scene == nullptr) || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || (scene->mRootNode == nullptr)) {
		throw std::runtime_error("Assimp failed: " + std::string(importer.GetErrorString()));
	}

	m_Path = filepath;
	m_Vertices.clear();
	m_Indices.clear();
	m_Primitives.clear();

	size_t totalVertices = 0, totalIndices = 0;
	Delegate<void(aiNode *)> Count = [&](aiNode *node) {
		for (uint32 i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			totalVertices += mesh->mNumVertices;
			totalIndices += mesh->mNumFaces * 3;
		}
		for (uint32 i = 0; i < node->mNumChildren; i++) {
			Count(node->mChildren[i]);
		}
	};
	Count(scene->mRootNode);

	m_Vertices.reserve(totalVertices);
	m_Indices.reserve(totalIndices);

	Delegate<void(aiNode *, const aiScene *)> ProcessNode;
	ProcessNode = [&](aiNode *node, const aiScene *s) {
		for (uint32 i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = s->mMeshes[node->mMeshes[i]];

			uint32 vertexOffset = static_cast<uint32>(m_Vertices.size());
			uint32 indexOffset = static_cast<uint32>(m_Indices.size());

			for (uint32 j = 0; j < mesh->mNumVertices; j++) {
				RHI::Vertex v{};
				v.pos = glm::vec4(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1.f);
				v.normals = mesh->HasNormals()
					? glm::normalize(vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z))
					: vec3(0, 1, 0);
				v.texcoord = mesh->HasTextureCoords(0)
					? vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y)
					: vec2(0);

				if (mesh->HasTangentsAndBitangents()) {
					vec3 T = glm::normalize(vec3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z));
					vec3 B =
						glm::normalize(vec3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z));
					vec3 N = glm::normalize(vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z));
					f32 h = (glm::dot(glm::cross(N, T), B) < 0.f) ? -1.f : 1.f;
					v.tangent = glm::vec4(T, h);
				} else {
					v.tangent = glm::vec4(1, 0, 0, 1);
				}
				m_Vertices.push_back(v);
			}

			for (uint32 j = 0; j < mesh->mNumFaces; j++) {
				const aiFace &face = mesh->mFaces[j];
				for (uint32 k = 0; k < face.mNumIndices; k++) {
					m_Indices.push_back(face.mIndices[k] + vertexOffset);
				}
			}

			RHI::GPUMeshPrimitive prim{};
			prim.firstVertex = vertexOffset;
			prim.vertexCount = mesh->mNumVertices;
			prim.firstIndex = indexOffset;
			prim.indexCount = mesh->mNumFaces * 3;
			m_Primitives.push_back(prim);
		}
		for (uint32 i = 0; i < node->mNumChildren; i++) {
			ProcessNode(node->mChildren[i], s);
		}
	};
	ProcessNode(scene->mRootNode, scene);

	CenterMeshAtOrigin();

	m_VertexCount = static_cast<uint32>(m_Vertices.size());
	m_IndexCount = static_cast<uint32>(m_Indices.size());
	m_HasIndexBuffer = m_IndexCount > 0;

	AQUILA_LOG_INFO("Loaded mesh '{}' - {} vertices, {} indices", m_DebugName, m_VertexCount, m_IndexCount);
}

void Mesh::LoadFromData(const MeshData &meshData) {
	if (meshData.vertices.empty()) {
		AQUILA_LOG_ERROR("Mesh data has no vertices: {}", m_DebugName);
		return;
	}

	m_Vertices = meshData.vertices;
	m_Indices = meshData.indices;
	m_Path = meshData.path;

	m_VertexCount = static_cast<uint32>(m_Vertices.size());
	m_IndexCount = static_cast<uint32>(m_Indices.size());
	m_HasIndexBuffer = m_IndexCount > 0;

	RHI::GPUMeshPrimitive prim{};
	prim.firstVertex = 0;
	prim.vertexCount = m_VertexCount;
	prim.firstIndex = 0;
	prim.indexCount = m_IndexCount;
	m_Primitives = { prim };

	AQUILA_LOG_INFO("Loaded mesh '{}' - {} vertices, {} indices", m_DebugName, m_VertexCount, m_IndexCount);
}

void Mesh::CenterMeshAtOrigin() {
	if (m_Vertices.empty()) {
		return;
	}

	vec3 minB = vec3(m_Vertices[0].pos);
	vec3 maxB = vec3(m_Vertices[0].pos);
	for (const auto &v : m_Vertices) {
		vec3 p = vec3(v.pos);
		minB = glm::min(minB, p);
		maxB = glm::max(maxB, p);
	}
	vec3 center = (minB + maxB) * 0.5f;
	for (auto &v : m_Vertices) {
		v.pos = vec4(vec3(v.pos) - center, 1.f);
	}

	AQUILA_LOG_INFO("Centered mesh '{}' by ({}, {}, {})", m_DebugName, center.x, center.y, center.z);
}

MeshData Mesh::GenerateCube(f32 size) {
	MeshData data;
	data.path = "procedural://cube";
	constexpr vec3 white = { 1, 1, 1 };

	data.vertices = {
		{ { -size, -size, size }, white, { 0, 0, 1 }, { 0, 0 }, { 1, 0, 0, 1 } },
		{ { size, -size, size }, white, { 0, 0, 1 }, { 1, 0 }, { 1, 0, 0, 1 } },
		{ { size, size, size }, white, { 0, 0, 1 }, { 1, 1 }, { 1, 0, 0, 1 } },
		{ { -size, size, size }, white, { 0, 0, 1 }, { 0, 1 }, { 1, 0, 0, 1 } },
		{ { size, -size, -size }, white, { 0, 0, -1 }, { 0, 0 }, { -1, 0, 0, 1 } },
		{ { -size, -size, -size }, white, { 0, 0, -1 }, { 1, 0 }, { -1, 0, 0, 1 } },
		{ { -size, size, -size }, white, { 0, 0, -1 }, { 1, 1 }, { -1, 0, 0, 1 } },
		{ { size, size, -size }, white, { 0, 0, -1 }, { 0, 1 }, { -1, 0, 0, 1 } },
		{ { -size, -size, -size }, white, { -1, 0, 0 }, { 0, 0 }, { 0, 0, 1, 1 } },
		{ { -size, -size, size }, white, { -1, 0, 0 }, { 1, 0 }, { 0, 0, 1, 1 } },
		{ { -size, size, size }, white, { -1, 0, 0 }, { 1, 1 }, { 0, 0, 1, 1 } },
		{ { -size, size, -size }, white, { -1, 0, 0 }, { 0, 1 }, { 0, 0, 1, 1 } },
		{ { size, -size, size }, white, { 1, 0, 0 }, { 0, 0 }, { 0, 0, -1, 1 } },
		{ { size, -size, -size }, white, { 1, 0, 0 }, { 1, 0 }, { 0, 0, -1, 1 } },
		{ { size, size, -size }, white, { 1, 0, 0 }, { 1, 1 }, { 0, 0, -1, 1 } },
		{ { size, size, size }, white, { 1, 0, 0 }, { 0, 1 }, { 0, 0, -1, 1 } },
		{ { -size, size, size }, white, { 0, 1, 0 }, { 0, 0 }, { 1, 0, 0, 1 } },
		{ { size, size, size }, white, { 0, 1, 0 }, { 1, 0 }, { 1, 0, 0, 1 } },
		{ { size, size, -size }, white, { 0, 1, 0 }, { 1, 1 }, { 1, 0, 0, 1 } },
		{ { -size, size, -size }, white, { 0, 1, 0 }, { 0, 1 }, { 1, 0, 0, 1 } },
		{ { -size, -size, -size }, white, { 0, -1, 0 }, { 0, 0 }, { 1, 0, 0, 1 } },
		{ { size, -size, -size }, white, { 0, -1, 0 }, { 1, 0 }, { 1, 0, 0, 1 } },
		{ { size, -size, size }, white, { 0, -1, 0 }, { 1, 1 }, { 1, 0, 0, 1 } },
		{ { -size, -size, size }, white, { 0, -1, 0 }, { 0, 1 }, { 1, 0, 0, 1 } },
	};
	data.indices = {
		0,	1,	2,	2,	3,	0,	4,	5,	6,	6,	7,	4,	8,	9,	10, 10, 11, 8,
		12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20,
	};
	return data;
}

MeshData Mesh::GenerateSphere(f32 radius, uint32 segments, uint32 rings) {
	MeshData data;
	data.path = "procedural://sphere";
	constexpr vec3 white = { 1, 1, 1 };

	data.vertices.push_back({ { 0, radius, 0 }, white, { 0, 1, 0 }, { 0.5f, 1.f }, { 1, 0, 0, 1 } });
	for (uint32 stack = 1; stack < rings; ++stack) {
		f32 phi = Math::PI * f32(stack) / f32(rings);
		for (uint32 slice = 0; slice < segments; ++slice) {
			f32 theta = 2.f * Math::PI * f32(slice) / f32(segments);
			vec3 pos = { radius * sin(phi) * cos(theta), radius * cos(phi), radius * sin(phi) * sin(theta) };
			data.vertices.push_back({ pos,
									  white,
									  glm::normalize(pos),
									  { f32(slice) / f32(segments), 1.f - f32(stack) / f32(rings) },
									  glm::normalize(vec4(-sin(theta), 0, cos(theta), 1)) });
		}
	}
	data.vertices.push_back({ { 0, -radius, 0 }, white, { 0, -1, 0 }, { 0.5f, 0.f }, { 1, 0, 0, 1 } });

	uint32 top = 0;
	uint32 bottom = static_cast<uint32>(data.vertices.size() - 1);

	for (uint32 i = 0; i < segments; ++i) {
		data.indices.push_back(top);
		data.indices.push_back(1 + (i + 1) % segments);
		data.indices.push_back(1 + i);
	}
	uint32 lastRing = 1 + (rings - 2) * segments;
	for (uint32 i = 0; i < segments; ++i) {
		data.indices.push_back(bottom);
		data.indices.push_back(lastRing + i);
		data.indices.push_back(lastRing + (i + 1) % segments);
	}
	for (uint32 stack = 0; stack < rings - 2; ++stack) {
		uint32 curr = 1 + stack * segments;
		uint32 next = curr + segments;
		for (uint32 slice = 0; slice < segments; ++slice) {
			uint32 i0 = curr + slice, i1 = curr + (slice + 1) % segments;
			uint32 i2 = next + (slice + 1) % segments, i3 = next + slice;
			data.indices.insert(data.indices.end(), { i0, i1, i2, i0, i2, i3 });
		}
	}
	return data;
}

MeshData Mesh::GenerateCylinder(f32 radius, f32 height, uint32 segments) {
	MeshData data;
	data.path = "procedural://cylinder";
	constexpr vec3 white = { 1, 1, 1 };
	f32 half = height * 0.5f;

	data.vertices.push_back({ { 0, half, 0 }, white, { 0, 1, 0 }, { 0.5f, 0.5f }, { 1, 0, 0, 1 } });
	data.vertices.push_back({ { 0, -half, 0 }, white, { 0, -1, 0 }, { 0.5f, 0.5f }, { 1, 0, 0, 1 } });

	constexpr uint32 sideStart = 2;
	for (uint32 i = 0; i <= segments; i++) {
		f32 angle = 2.f * Math::PI * i / segments;
		f32 x = radius * cos(angle), z = radius * sin(angle);
		vec3 n = glm::normalize(vec3(x, 0, z));
		vec4 t = glm::normalize(vec4(-sin(angle), 0, cos(angle), 1));
		f32 u = f32(i) / segments;
		data.vertices.push_back({ { x, half, z }, white, n, { u, 0 }, t });
		data.vertices.push_back({ { x, -half, z }, white, n, { u, 1 }, t });
	}
	for (uint32 i = 0; i < segments; i++) {
		uint32 tc = sideStart + i * 2, bc = tc + 1, tn = sideStart + (i + 1) * 2, bn = tn + 1;
		data.indices.insert(data.indices.end(), { tc, tn, bc, tn, bn, bc });
	}
	for (uint32 i = 0; i < segments; i++) {
		uint32 curr = sideStart + i * 2, next = sideStart + ((i + 1) % segments) * 2;
		data.indices.insert(data.indices.end(), { 0u, next, curr });
	}
	for (uint32 i = 0; i < segments; i++) {
		uint32 curr = sideStart + i * 2 + 1, next = sideStart + ((i + 1) % segments) * 2 + 1;
		data.indices.insert(data.indices.end(), { 1u, curr, next });
	}
	return data;
}

MeshData Mesh::GeneratePlane(f32 width, f32 height, uint32 wSegs, uint32 hSegs) {
	MeshData data;
	data.path = "procedural://plane";
	constexpr vec3 white = { 1, 1, 1 };

	for (uint32 y = 0; y <= hSegs; y++) {
		for (uint32 x = 0; x <= wSegs; x++) {
			f32 u = f32(x) / wSegs, v = f32(y) / hSegs;
			data.vertices.push_back(
				{ { (u - 0.5f) * width, 0, (v - 0.5f) * height }, white, { 0, 1, 0 }, { u, v }, { 1, 0, 0, 1 } });
		}
	}
	for (uint32 y = 0; y < hSegs; y++) {
		for (uint32 x = 0; x < wSegs; x++) {
			uint32 tl = y * (wSegs + 1) + x, tr = tl + 1;
			uint32 bl = (y + 1) * (wSegs + 1) + x, br = bl + 1;
			data.indices.insert(data.indices.end(), { tl, bl, tr, tr, bl, br });
		}
	}
	return data;
}

} // namespace Aquila::Graphics::Resources
