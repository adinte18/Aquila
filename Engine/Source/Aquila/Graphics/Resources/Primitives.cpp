// #include "Aquila/Graphics/Resources/Primitives.h"

// namespace Aquila::Graphics::Resources {
// std::vector<Vertex> Primitives::CreateCube(const f32 size) {
// 	std::vector<Vertex> vertices;
// 	f32 halfSize = size / 2.0f;

// 	vec3 positions[8] = {
// 		{ -halfSize, -halfSize, -halfSize }, // 0
// 		{ halfSize, -halfSize, -halfSize },	 // 1
// 		{ halfSize, halfSize, -halfSize },	 // 2
// 		{ -halfSize, halfSize, -halfSize },	 // 3
// 		{ -halfSize, -halfSize, halfSize },	 // 4
// 		{ halfSize, -halfSize, halfSize },	 // 5
// 		{ halfSize, halfSize, halfSize },	 // 6
// 		{ -halfSize, halfSize, halfSize }	 // 7
// 	};

// 	struct Face {
// 		int indices[6];
// 		vec3 normal;
// 		vec4 tangent;
// 	};

// 	Face faces[6] = {
// 		// Front (z+)
// 		{ { 4, 5, 6, 6, 7, 4 }, { 0, 0, 1 }, { 1, 0, 0, 1 } }, // Tangent aligned with X axis
// 		// Back (z-)
// 		{ { 0, 1, 2, 2, 3, 0 }, { 0, 0, -1 }, { -1, 0, 0 } }, // Tangent aligned with -X axis
// 		// Left (x-)
// 		{ { 0, 4, 7, 7, 3, 0 }, { -1, 0, 0 }, { 0, 0, 1 } }, // Tangent aligned with Z axis
// 		// Right (x+)
// 		{ { 1, 5, 6, 6, 2, 1 }, { 1, 0, 0 }, { 0, 0, -1 } }, // Tangent aligned with -Z axis
// 		// Top (y+)
// 		{ { 3, 7, 6, 6, 2, 3 }, { 0, 1, 0 }, { 1, 0, 0 } }, // Tangent aligned with X axis
// 		// Bottom (y-)
// 		{ { 0, 4, 5, 5, 1, 0 }, { 0, -1, 0 }, { -1, 0, 0 } } // Tangent aligned with -X axis
// 	};

// 	for (const auto &face : faces) {
// 		for (int i = 0; i < 6; ++i) {
// 			vec2 texcoords[6][6] = { // Front (z+)
// 									 { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 1, 1 }, { 0, 1 }, { 0, 0 } },
// 									 // Back (z-)
// 									 { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 1, 1 }, { 1, 0 } },
// 									 // Left (x-)
// 									 { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 0, 1 }, { 1, 1 }, { 1, 0 } },
// 									 // Right (x+)
// 									 { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 1, 1 }, { 0, 1 }, { 0, 0 } },
// 									 // Top (y+)
// 									 { { 0, 1 }, { 0, 0 }, { 1, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } },
// 									 // Bottom (y-)
// 									 { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, 0 }, { 0, 0 }, { 0, 1 } }
// 			};
// 			Vertex vertex;
// 			int idx = face.indices[i];
// 			vertex.pos = positions[idx];
// 			vertex.normals = face.normal;
// 			vertex.texcoord = texcoords[&face - faces][i];
// 			vertex.color = { 1.0f, 1.0f, 1.0f };
// 			vertex.tangent = face.tangent;
// 			vertices.push_back(vertex);
// 		}
// 	}

// 	return vertices;
// }

// std::vector<Vertex> Primitives::CreateSphere(const f32 radius, const int latitudeSegments,
// 											 const int longitudeSegments) {
// 	std::vector<Vertex> vertices;

// 	for (int lat = 0; lat <= latitudeSegments; ++lat) {
// 		f32 theta = lat * glm::pi<f32>() / latitudeSegments;
// 		f32 sinTheta = sin(theta);
// 		f32 cosTheta = cos(theta);

// 		for (int lon = 0; lon <= longitudeSegments; ++lon) {
// 			f32 phi = lon * 2.0f * glm::pi<f32>() / longitudeSegments;
// 			f32 sinPhi = sin(phi);
// 			f32 cosPhi = cos(phi);

// 			Vertex vertex;

// 			vertex.pos = vec3(radius * cosPhi * sinTheta, radius * cosTheta, radius * sinPhi * sinTheta);

// 			vertex.normals = glm::normalize(vertex.pos);

// 			vertex.texcoord = vec2(static_cast<f32>(lon) / longitudeSegments, // u
// 								   static_cast<f32>(lat) / latitudeSegments	  // v
// 			);

// 			vertex.color = vec3(1.0f);

// 			vertices.push_back(vertex);
// 		}
// 	}

// 	std::vector<int> indices;
// 	for (int lat = 0; lat < latitudeSegments; ++lat) {
// 		for (int lon = 0; lon < longitudeSegments; ++lon) {
// 			int first = (lat * (longitudeSegments + 1)) + lon;
// 			int second = first + longitudeSegments + 1;

// 			indices.push_back(first);
// 			indices.push_back(second);
// 			indices.push_back(first + 1);

// 			indices.push_back(second);
// 			indices.push_back(second + 1);
// 			indices.push_back(first + 1);
// 		}
// 	}

// 	std::vector<Vertex> finalVertices;
// 	for (int index : indices) {
// 		finalVertices.push_back(vertices[index]);
// 	}

// 	return finalVertices;
// }
// } // namespace Aquila::Graphics::Resources
