#include "Engine/Primitives.h"

#include <glm/ext/scalar_constants.hpp>
#define M_PI 3.14159265358979323846

namespace Engine {
std::vector<Vertex> Primitives::CreateCube(float size) {
    std::vector<Vertex> vertices;
    float halfSize = size / 2.0f;

    // Define the positions of the 8 vertices of the cube
    glm::vec3 positions[8] = {
        {-halfSize, -halfSize, -halfSize}, // 0
        { halfSize, -halfSize, -halfSize}, // 1
        { halfSize,  halfSize, -halfSize}, // 2
        {-halfSize,  halfSize, -halfSize}, // 3
        {-halfSize, -halfSize,  halfSize}, // 4
        { halfSize, -halfSize,  halfSize}, // 5
        { halfSize,  halfSize,  halfSize}, // 6
        {-halfSize,  halfSize,  halfSize}  // 7
    };

    // Define texcoords for each face of the cube
    glm::vec2 texcoords[6][6] = {
        // Front (z+)
        {{0, 0}, {1, 0}, {1, 1}, {1, 1}, {0, 1}, {0, 0}},
        // Back (z-)
        {{1, 0}, {0, 0}, {0, 1}, {0, 1}, {1, 1}, {1, 0}},
        // Left (x-)
        {{1, 0}, {0, 0}, {0, 1}, {0, 1}, {1, 1}, {1, 0}},
        // Right (x+)
        {{0, 0}, {1, 0}, {1, 1}, {1, 1}, {0, 1}, {0, 0}},
        // Top (y+)
        {{0, 1}, {0, 0}, {1, 0}, {1, 0}, {1, 1}, {0, 1}},
        // Bottom (y-)
        {{0, 1}, {1, 1}, {1, 0}, {1, 0}, {0, 0}, {0, 1}}
    };

    // Define the faces (indices and normals)
    struct Face {
        int indices[6];
        glm::vec3 normal;
        glm::vec3 tangent;
    };

    Face faces[6] = {
        // Front (z+)
        {{4, 5, 6, 6, 7, 4}, {0, 0, 1}, {1, 0, 0}}, // Tangent aligned with X axis
        // Back (z-)
        {{0, 1, 2, 2, 3, 0}, {0, 0, -1}, {-1, 0, 0}}, // Tangent aligned with -X axis
        // Left (x-)
        {{0, 4, 7, 7, 3, 0}, {-1, 0, 0}, {0, 0, 1}}, // Tangent aligned with Z axis
        // Right (x+)
        {{1, 5, 6, 6, 2, 1}, {1, 0, 0}, {0, 0, -1}}, // Tangent aligned with -Z axis
        // Top (y+)
        {{3, 7, 6, 6, 2, 3}, {0, 1, 0}, {1, 0, 0}}, // Tangent aligned with X axis
        // Bottom (y-)
        {{0, 4, 5, 5, 1, 0}, {0, -1, 0}, {-1, 0, 0}} // Tangent aligned with -X axis
    };

    // Populate the vertices for each face
    for (const auto& face : faces) {
        for (int i = 0; i < 6; ++i) {
            Vertex vertex;
            int idx = face.indices[i];
            vertex.pos = positions[idx];
            vertex.normals = face.normal;
            vertex.texcoord = texcoords[&face - faces][i]; // Use texcoords specific to each face
            vertex.color = {1.0f, 1.0f, 1.0f}; // Default white color
            vertex.tangent = face.tangent; // Set the tangent
            // Push vertex to the vertices vector
            vertices.push_back(vertex);
        }
    }

    return vertices;
}



    std::vector<Vertex> Primitives::CreateSphere(float radius, int latitudeSegments, int longitudeSegments) {
        std::vector<Vertex> vertices;

        for (int lat = 0; lat <= latitudeSegments; ++lat) {
            float theta = lat * glm::pi<float>() / latitudeSegments;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            for (int lon = 0; lon <= longitudeSegments; ++lon) {
                float phi = lon * 2.0f * glm::pi<float>() / longitudeSegments;
                float sinPhi = sin(phi);
                float cosPhi = cos(phi);

                Vertex vertex;

                // Position
                vertex.pos = glm::vec3(
                    radius * cosPhi * sinTheta,
                    radius * cosTheta,
                    radius * sinPhi * sinTheta
                );

                // Normal
                vertex.normals = glm::normalize(vertex.pos);

                // Texture Coordinates
                vertex.texcoord = glm::vec2(
                    static_cast<float>(lon) / longitudeSegments,  // u
                    static_cast<float>(lat) / latitudeSegments    // v
                );

                // Color (for debugging purposes, optional)
                vertex.color = glm::vec3(1.0f); // Set to white by default

                vertices.push_back(vertex);
            }
        }

        // Generate indices for triangle strips
        std::vector<int> indices;
        for (int lat = 0; lat < latitudeSegments; ++lat) {
            for (int lon = 0; lon < longitudeSegments; ++lon) {
                int first = (lat * (longitudeSegments + 1)) + lon;
                int second = first + longitudeSegments + 1;

                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        // Create the final vertex array using indexed vertices
        std::vector<Vertex> finalVertices;
        for (int index : indices) {
            finalVertices.push_back(vertices[index]);
        }

        return finalVertices;
    }
}
