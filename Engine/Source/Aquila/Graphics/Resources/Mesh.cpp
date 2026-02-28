
#include "Aquila/Graphics/Resources/Mesh.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"

namespace Aquila::Graphics::Resources {
Mesh::Mesh(Device &device, const std::string &debugName)
	: m_DebugName(debugName), m_Device(device), m_VertexCount(0), m_IndexCount(0) {}

Mesh::~Mesh() = default;

void Mesh::LoadData(const std::string &filepath) {
	Assimp::Importer importer;

	const auto file = Platform::Filesystem::VirtualFileSystem::Get()->OpenFile(filepath, "rb");
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

	unsigned int flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality;

	if (fileSize < 5 * 1024 * 1024) { // < 5MB
		flags |= aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_GenUVCoords;
	}

	const aiScene *scene = importer.ReadFileFromMemory(buffer.data(), buffer.size(), flags);

	if ((scene == nullptr) || ((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U) || (scene->mRootNode == nullptr)) {
		throw std::runtime_error("Assimp failed to load model: " + std::string(importer.GetErrorString()));
	}

	m_Path = filepath;

	size_t totalVertices = 0;
	size_t totalIndices = 0;

	std::function<void(aiNode *)> CountMeshes = [&](aiNode *node) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			totalVertices += mesh->mNumVertices;
			totalIndices += mesh->mNumFaces * 3;
		}
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			CountMeshes(node->mChildren[i]);
		}
	};

	CountMeshes(scene->mRootNode);

	// Reserve memory ONCE
	m_Vertices.clear();
	m_Indices.clear();
	m_Primitives.clear();
	m_Vertices.reserve(totalVertices);
	m_Indices.reserve(totalIndices);

	Delegate<void(aiNode *, const aiScene *)> ProcessNode;

	ProcessNode = [&](aiNode *node, const aiScene *asimpScene) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = asimpScene->mMeshes[node->mMeshes[i]];

			uint32 vertexOffset = m_Vertices.size();
			uint32 indexOffset = m_Indices.size();

			// Process vertices
			for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
				Vertex vertex{};
				vertex.pos = glm::vec4(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z, 1.0f);
				vertex.normals =
					mesh->HasNormals()
						? glm::normalize(vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z))
						: vec3(0.0f, 1.0f, 0.0f);
				vertex.texcoord = mesh->HasTextureCoords(0)
									  ? vec2(mesh->mTextureCoords[0][j].x, mesh->mTextureCoords[0][j].y)
									  : vec2(0.0f);

				if (mesh->HasTangentsAndBitangents()) {
					glm::vec3 T =
						glm::normalize(vec3(mesh->mTangents[j].x, mesh->mTangents[j].y, mesh->mTangents[j].z));
					glm::vec3 B =
						glm::normalize(vec3(mesh->mBitangents[j].x, mesh->mBitangents[j].y, mesh->mBitangents[j].z));
					glm::vec3 N = glm::normalize(vec3(mesh->mNormals[j].x, mesh->mNormals[j].y, mesh->mNormals[j].z));
					f32 handedness = (glm::dot(glm::cross(N, T), B) < 0.0f) ? -1.0f : 1.0f;
					vertex.tangent = glm::vec4(T, handedness);
				} else {
					vertex.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
				}

				m_Vertices.push_back(vertex);
			}

			// Process indices
			for (unsigned int j = 0; j < mesh->mNumFaces; j++) {
				const aiFace &face = mesh->mFaces[j];
				for (unsigned int k = 0; k < face.mNumIndices; k++) {
					m_Indices.push_back(face.mIndices[k] + vertexOffset);
				}
			}

			Primitive prim{};
			prim.firstVertex = vertexOffset;
			prim.vertexCount = mesh->mNumVertices;
			prim.indexCount = mesh->mNumFaces * 3;
			prim.firstIndex = indexOffset;
			m_Primitives.push_back(prim);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ProcessNode(node->mChildren[i], asimpScene);
		}
	};

	ProcessNode(scene->mRootNode, scene);

	CenterMeshAtOrigin();

	m_VertexCount = static_cast<uint32>(m_Vertices.size());
	m_IndexCount = static_cast<uint32>(m_Indices.size());
	m_HasIndexBuffer = m_IndexCount > 0;

	AQUILA_LOG_INFO("Loaded mesh data '{}' - {} vertices, {} indices", m_DebugName, m_VertexCount, m_IndexCount);
}

// Keep the old Load() method for backward compatibility
void Mesh::Load(const std::string &filepath) {
	LoadData(filepath);

	// Create buffers immediately (old behavior)
	CreateVertexBuffer(m_Vertices);
	if (m_HasIndexBuffer) {
		CreateIndexBuffer(m_Indices);
	}

	m_GPUResourcesCreated = true;
}

void Mesh::CenterMeshAtOrigin() {
	if (m_Vertices.empty()) {
		return;
	}

	vec3 minBounds = vec3(m_Vertices[0].pos);
	vec3 maxBounds = vec3(m_Vertices[0].pos);

	for (const auto &vertex : m_Vertices) {
		vec3 pos = vec3(vertex.pos);
		minBounds = glm::min(minBounds, pos);
		maxBounds = glm::max(maxBounds, pos);
	}

	vec3 center = (minBounds + maxBounds) * 0.5f;

	for (auto &vertex : m_Vertices) {
		vertex.pos = vec4(vec3(vertex.pos) - center, 1.0f);
	}

	AQUILA_LOG_INFO("Centered mesh '{}' by offset: ({}, {}, {})", m_DebugName, center.x, center.y, center.z);
}

void Mesh::UploadToGPU(VkCommandBuffer cmd) {
	if (m_GPUResourcesCreated) {
		AQUILA_LOG_WARNING("GPU resources already created for mesh: {}", m_DebugName);
		return;
	}

	if (m_Vertices.empty()) {
		AQUILA_LOG_WARNING("No vertex data to upload for mesh: {}", m_DebugName);
		return;
	}

	CreateVertexBufferWithCmd(cmd, m_Vertices);

	if (m_HasIndexBuffer && !m_Indices.empty()) {
		CreateIndexBufferWithCmd(cmd, m_Indices);
	}

	m_GPUResourcesCreated = true;

	AQUILA_LOG_DEBUG("Uploaded mesh to GPU: {}", m_DebugName);
}

// New: Create vertex buffer using provided command buffer
void Mesh::CreateVertexBufferWithCmd(VkCommandBuffer cmd, const std::vector<Vertex> &vertices) {
	m_VertexCount = static_cast<uint32>(vertices.size());
	AQUILA_ASSERT(m_VertexCount >= 3, "Vertex count must be at least 3");

	VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
	uint32 vertexSize = sizeof(vertices[0]);

	m_VertexStagingBuffer = CreateUnique<Buffer>(
		m_Device, std::string(m_DebugName + "_VertexStagingBuffer"), vertexSize, m_VertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_VertexStagingBuffer->Map();
	m_VertexStagingBuffer->Write((void *)vertices.data());
	m_VertexStagingBuffer->UnMap();

	m_VertexBuffer = CreateUnique<Buffer>(
		m_Device, std::string(m_DebugName + "_VertexBuffer"), vertexSize, m_VertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferSize;

	vkCmdCopyBuffer(cmd, m_VertexStagingBuffer->GetBuffer(), m_VertexBuffer->GetBuffer(), 1, &copyRegion);
}

// New: Create index buffer using provided command buffer
void Mesh::CreateIndexBufferWithCmd(VkCommandBuffer cmd, const std::vector<uint32> &indices) {
	m_IndexCount = static_cast<uint32>(indices.size());
	m_HasIndexBuffer = m_IndexCount > 0;

	if (!m_HasIndexBuffer) {
		return;
	}

	const VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
	uint32 indexSize = sizeof(indices[0]);

	// Create staging buffer and KEEP IT ALIVE
	m_IndexStagingBuffer = CreateUnique<Buffer>(
		m_Device, std::string(m_DebugName + "_IndexStagingBuffer"), indexSize, m_IndexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	m_IndexStagingBuffer->Map();
	m_IndexStagingBuffer->Write((void *)indices.data());
	m_IndexStagingBuffer->UnMap();

	// Create device-local buffer
	m_IndexBuffer = CreateUnique<Buffer>(m_Device, std::string(m_DebugName + "_IndexBuffer"), indexSize, m_IndexCount,
										 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Copy using provided command buffer
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = bufferSize;

	vkCmdCopyBuffer(cmd, m_IndexStagingBuffer->GetBuffer(), m_IndexBuffer->GetBuffer(), 1, &copyRegion);
}

void Mesh::CreateVertexBuffer(const std::vector<Vertex> &vertices) {
	m_VertexCount = static_cast<uint32>(vertices.size());
	AQUILA_ASSERT(m_VertexCount >= 3, "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
	uint32 vertexSize = sizeof(vertices[0]);

	Buffer stagingBuffer{ m_Device,
						  std::string(m_DebugName + "_VertexStagingBuffer"),
						  vertexSize,
						  m_VertexCount,
						  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	stagingBuffer.Map();
	stagingBuffer.Write((void *)vertices.data());

	m_VertexBuffer = CreateUnique<Buffer>(
		m_Device, std::string(m_DebugName + "_VertexBuffer"), vertexSize, m_VertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
}

void Mesh::UpdateVertexBuffer(const std::vector<Vertex> &vertices) const {
	if (!m_VertexBuffer) {
		throw std::runtime_error("Cannot update vertex buffer: Buffer not created!");
	}

	const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	constexpr uint32 vertexSize = sizeof(vertices[0]);

	Buffer stagingBuffer{ m_Device,
						  std::string(m_DebugName + "_VertexStagingBuffer"),
						  vertexSize,
						  static_cast<uint32>(vertices.size()),
						  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	stagingBuffer.Map();
	stagingBuffer.Write(vertices.data());

	m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
}

void Mesh::CreateIndexBuffer(const std::vector<uint32> &indices) {
	m_IndexCount = static_cast<uint32>(indices.size());
	m_HasIndexBuffer = m_IndexCount > 0;

	if (!m_HasIndexBuffer) {
		return;
	}

	const VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
	uint32 indexSize = sizeof(indices[0]);

	Buffer stagingBuffer{ m_Device,
						  std::string(m_DebugName + "_IndexStagingBuffer"),
						  indexSize,
						  m_IndexCount,
						  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

	stagingBuffer.Map();
	stagingBuffer.Write((void *)indices.data());

	m_IndexBuffer = CreateUnique<Buffer>(m_Device, std::string(m_DebugName + "_IndexBuffer"), indexSize, m_IndexCount,
										 VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
}

void Mesh::LoadFromData(const MeshData &meshData) {
	if (meshData.vertices.empty()) {
		AQUILA_LOG_ERROR("Mesh data contains no vertices: {}", m_DebugName);
		return;
	}

	m_Vertices = meshData.vertices;
	m_Indices = meshData.indices;
	m_Path = meshData.path;

	m_VertexCount = static_cast<uint32>(meshData.vertices.size());

	CreateVertexBuffer(meshData.vertices);

	if (!meshData.indices.empty()) {
		m_HasIndexBuffer = true;
		m_IndexCount = static_cast<uint32>(meshData.indices.size());
		CreateIndexBuffer(meshData.indices);
	} else {
		m_HasIndexBuffer = false;
		m_IndexCount = 0;
	}

	if (!m_Vertices.empty()) {
		Primitive primitive{};
		primitive.firstVertex = 0;
		primitive.vertexCount = m_VertexCount;

		if (m_HasIndexBuffer) {
			primitive.firstIndex = 0;
			primitive.indexCount = m_IndexCount;
		} else {
			primitive.firstIndex = 0;
			primitive.indexCount = 0;
		}

		m_Primitives.clear();
		m_Primitives.push_back(primitive);
	}

	AQUILA_LOG_INFO("Loaded mesh data '{}' - {} vertices, {} indices", m_DebugName, m_VertexCount, m_IndexCount);
}

void Mesh::Draw(const VkCommandBuffer commandBuffer) {
	if (m_Primitives.empty()) {
		return;
	}

	for (auto &[firstIndex, firstVertex, indexCount, vertexCount] : m_Primitives) {
		if (m_HasIndexBuffer) {
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, firstIndex, firstVertex, 0);
		} else {
			vkCmdDraw(commandBuffer, vertexCount, 1, firstVertex, 0);
		}
	}
}

MeshData Mesh::GenerateCube(f32 size) {
	MeshData meshData;
	meshData.path = "procedural://cube";

	constexpr vec3 white = { 1.0f, 1.0f, 1.0f };

	meshData.vertices = {
		{ { -size, -size, size }, white, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { size, -size, size }, white, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { size, size, size }, white, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -size, size, size }, white, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },

		{ { size, -size, -size }, white, { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -size, -size, -size }, white, { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f }, { -1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -size, size, -size }, white, { 0.0f, 0.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 0.0f, 0.0f, 1.0f } },
		{ { size, size, -size }, white, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f, 0.0f, 1.0f } },

		{ { -size, -size, -size }, white, { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -size, -size, size }, white, { -1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -size, size, size }, white, { -1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -size, size, -size }, white, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },

		{ { size, -size, size }, white, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f, 1.0f } },
		{ { size, -size, -size }, white, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f, 1.0f } },
		{ { size, size, -size }, white, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f, 1.0f } },
		{ { size, size, size }, white, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f, 1.0f } },

		{ { -size, size, size }, white, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { size, size, size }, white, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { size, size, -size }, white, { 0.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -size, size, -size }, white, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },

		{ { -size, -size, -size }, white, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { size, -size, -size }, white, { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { size, -size, size }, white, { 0.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -size, -size, size }, white, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }
	};

	meshData.indices = { 0,	 1,	 2,	 2,	 3,	 0,	 4,	 5,	 6,	 6,	 7,	 4,	 8,	 9,	 10, 10, 11, 8,
						 12, 13, 14, 14, 15, 12, 16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20 };

	return meshData;
}

MeshData Mesh::GenerateSphere(f32 radius, uint32 segments, uint32 rings) {
	MeshData meshData;
	meshData.path = "procedural://sphere";

	constexpr vec3 white = { 1.0f, 1.0f, 1.0f };

	//https://danielsieger.com/blog/2021/03/27/generating-spheres.html

	// Top vertex
	{
		vec3 pos = { 0.0f, radius, 0.0f };
		vec3 normal = { 0.0f, 1.0f, 0.0f };
		vec2 uv = { 0.5f, 1.0f };
		vec4 tangent = { 1.0f, 0.0f, 0.0f, 1.0f };

		meshData.vertices.push_back({ pos, white, normal, uv, tangent });
	}

	//  Middle stacks
	for (uint32 stack = 1; stack < rings; ++stack) {
		f32 phi = Math::PI * f32(stack) / f32(rings);

		for (uint32 slice = 0; slice < segments; ++slice) {
			f32 theta = 2.0f * Math::PI * f32(slice) / f32(segments);

			vec3 pos = { radius * sin(phi) * cos(theta), radius * cos(phi), radius * sin(phi) * sin(theta) };

			vec3 normal = glm::normalize(pos);
			vec2 uv = { f32(slice) / f32(segments), 1.0f - f32(stack) / f32(rings) };

			vec4 tangent = glm::normalize(vec4(-sin(theta), 0.0f, cos(theta), 1.0f));

			meshData.vertices.push_back({ pos, white, normal, uv, tangent });
		}
	}

	// Bottom vertex
	{
		vec3 pos = { 0.0f, -radius, 0.0f };
		vec3 normal = { 0.0f, -1.0f, 0.0f };
		vec2 uv = { 0.5f, 0.0f };
		vec4 tangent = { 1.0f, 0.0f, 0.0f, 1.0f };

		meshData.vertices.push_back({ pos, white, normal, uv, tangent });
	}

	uint32 topIndex = 0;
	uint32 bottomIndex = static_cast<uint32>(meshData.vertices.size() - 1);

	// Top cap
	for (uint32 i = 0; i < segments; ++i) {
		uint32 i0 = 1 + i;
		uint32 i1 = 1 + (i + 1) % segments;

		meshData.indices.push_back(topIndex);
		meshData.indices.push_back(i1);
		meshData.indices.push_back(i0);
	}

	// Bottom cap
	uint32 lastRingStart = 1 + (rings - 2) * segments;
	for (uint32 i = 0; i < segments; ++i) {
		uint32 i0 = lastRingStart + i;
		uint32 i1 = lastRingStart + (i + 1) % segments;

		meshData.indices.push_back(bottomIndex);
		meshData.indices.push_back(i0);
		meshData.indices.push_back(i1);
	}

	// Middle quads
	for (uint32 stack = 0; stack < rings - 2; ++stack) {
		uint32 curr = 1 + stack * segments;
		uint32 next = curr + segments;

		for (uint32 slice = 0; slice < segments; ++slice) {
			uint32 i0 = curr + slice;
			uint32 i1 = curr + (slice + 1) % segments;
			uint32 i2 = next + (slice + 1) % segments;
			uint32 i3 = next + slice;

			// first triangle
			meshData.indices.push_back(i0);
			meshData.indices.push_back(i1);
			meshData.indices.push_back(i2);

			// second triangle
			meshData.indices.push_back(i0);
			meshData.indices.push_back(i2);
			meshData.indices.push_back(i3);
		}
	}

	return meshData;
}

MeshData Mesh::GenerateCylinder(const f32 radius, const f32 height, const uint32 segments) {
	MeshData meshData;
	meshData.path = "procedural://cylinder";

	constexpr vec3 white = { 1.0f, 1.0f, 1.0f };
	f32 halfHeight = height * 0.5f;

	// Top center
	meshData.vertices.push_back({ { 0.0f, halfHeight, 0.0f }, white, { 0, 1, 0 }, { 0.5f, 0.5f }, { 1, 0, 0, 1 } });

	// Bottom center
	meshData.vertices.push_back({ { 0.0f, -halfHeight, 0.0f }, white, { 0, -1, 0 }, { 0.5f, 0.5f }, { 1, 0, 0, 1 } });

	// Side vertices
	constexpr uint32 sideStart = 2;
	for (uint32 i = 0; i <= segments; i++) {
		const f32 angle = 2.0f * Math::PI * i / segments;
		f32 x = radius * cos(angle);
		f32 z = radius * sin(angle);

		const vec3 normal = glm::normalize(vec3(x, 0, z));
		const vec4 tangent = glm::normalize(vec4(-sin(angle), 0, cos(angle), 1.0f));
		f32 u = static_cast<f32>(i) / segments;

		// top and bottom vertices
		meshData.vertices.push_back({ { x, halfHeight, z }, white, normal, { u, 0 }, tangent });
		meshData.vertices.push_back({ { x, -halfHeight, z }, white, normal, { u, 1 }, tangent });
	}

	// Indices for side quads (CCW)
	for (uint32 i = 0; i < segments; i++) {
		uint32 topCurr = sideStart + i * 2;
		uint32 bottomCurr = sideStart + i * 2 + 1;
		uint32 topNext = sideStart + (i + 1) * 2;
		uint32 bottomNext = sideStart + (i + 1) * 2 + 1;

		meshData.indices.push_back(topCurr);
		meshData.indices.push_back(topNext);
		meshData.indices.push_back(bottomCurr);

		meshData.indices.push_back(topNext);
		meshData.indices.push_back(bottomNext);
		meshData.indices.push_back(bottomCurr);
	}

	// Indices for top cap (CCW, viewed from above)
	for (uint32 i = 0; i < segments; i++) {
		uint32 topCenter = 0;
		uint32 curr = sideStart + i * 2; // top vertex
		uint32 next = sideStart + ((i + 1) % segments) * 2;
		meshData.indices.push_back(topCenter);
		meshData.indices.push_back(next);
		meshData.indices.push_back(curr);
	}

	// Indices for bottom cap (CCW, viewed from below)
	for (uint32 i = 0; i < segments; i++) {
		uint32 bottomCenter = 1;
		uint32 curr = sideStart + i * 2 + 1; // bottom vertex
		uint32 next = sideStart + ((i + 1) % segments) * 2 + 1;
		meshData.indices.push_back(bottomCenter);
		meshData.indices.push_back(curr);
		meshData.indices.push_back(next);
	}

	return meshData;
}

MeshData Mesh::GeneratePlane(f32 width, f32 height, uint32 widthSegments, uint32 heightSegments) {
	MeshData meshData;
	meshData.path = "procedural://plane";

	constexpr vec3 white = { 1.0f, 1.0f, 1.0f };

	for (uint32 y = 0; y <= heightSegments; y++) {
		for (uint32 x = 0; x <= widthSegments; x++) {
			f32 u = static_cast<f32>(x) / widthSegments;
			f32 v = static_cast<f32>(y) / heightSegments;

			const vec3 position = { (u - 0.5f) * width, 0.0f, (v - 0.5f) * height };

			meshData.vertices.push_back(
				{ position, white, { 0.0f, 1.0f, 0.0f }, { u, v }, { 1.0f, 0.0f, 0.0f, 1.0f } });
		}
	}

	for (uint32 y = 0; y < heightSegments; y++) {
		for (uint32 x = 0; x < widthSegments; x++) {
			uint32 topLeft = y * (widthSegments + 1) + x;
			uint32 topRight = topLeft + 1;
			uint32 bottomLeft = (y + 1) * (widthSegments + 1) + x;
			uint32 bottomRight = bottomLeft + 1;

			meshData.indices.insert(meshData.indices.end(),
									{ topLeft, bottomLeft, topRight, topRight, bottomLeft, bottomRight });
		}
	}

	return meshData;
}

void Mesh::Bind(const VkCommandBuffer commandBuffer) const {
	const VkBuffer buffers[] = { m_VertexBuffer->GetBuffer() };
	constexpr VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (m_HasIndexBuffer) {
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

}; // namespace Aquila::Graphics::Resources
