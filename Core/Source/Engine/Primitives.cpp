#include "Engine/Primitives.h"

#include <glm/ext/scalar_constants.hpp>
#define M_PI 3.14159265358979323846

namespace Engine {
    std::vector<Vertex> Primitives::CreateCube(float size) {
        std::vector<Vertex> vertices;
        float halfSize = size / 2.0f;

        glm::vec3 positions[8] = {
            {-halfSize, -halfSize, -halfSize},
            {halfSize, -halfSize, -halfSize},
            {halfSize, halfSize, -halfSize},
            {-halfSize, halfSize, -halfSize},
            {-halfSize, -halfSize, halfSize},
            {halfSize, -halfSize, halfSize},
            {halfSize, halfSize, halfSize},
            {-halfSize, halfSize, halfSize}
        };

        int indices[36] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            0, 1, 5, 5, 4, 0,
            2, 3, 7, 7, 6, 2,
            0, 3, 7, 7, 4, 0,
            1, 2, 6, 6, 5, 1
        };

        for (int indice : indices) {
            Vertex vertex;
            vertex.pos = positions[indice];
            vertices.push_back(vertex);
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
                vertex.pos = glm::vec3(
                    radius * cosPhi * sinTheta,
                    radius * cosTheta,
                    radius * sinPhi * sinTheta
                );
                vertices.push_back(vertex);
            }
        }

        // Generate indices
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

        // Create final vertex array
        std::vector<Vertex> finalVertices;
        for (int index : indices) {
            finalVertices.push_back(vertices[index]);
        }

        return finalVertices;
    }

    std::vector<Vertex> Primitives::CreateCylinder(float radius, float height, int segments) {
        std::vector<Vertex> vertices;

        float halfHeight = height / 2.0f;

        // Create top and bottom circle vertices
        for (int i = 0; i <= segments; ++i) {
            float theta = i * 2.0f * glm::pi<float>() / segments;
            float x = radius * cos(theta);
            float z = radius * sin(theta);

            Vertex topVertex, bottomVertex;
            topVertex.pos = glm::vec3(x, halfHeight, z);
            bottomVertex.pos = glm::vec3(x, -halfHeight, z);

            vertices.push_back(topVertex);
            vertices.push_back(bottomVertex);
        }

        // Generate indices for sides
        std::vector<int> indices;
        for (int i = 0; i < segments * 2; i += 2) {
            int next = (i + 2) % (segments * 2);

            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(next);

            indices.push_back(i + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }

        // Generate indices for top and bottom
        for (int i = 2; i < segments * 2; i += 2) {
            indices.push_back(0);
            indices.push_back(i);
            indices.push_back(i + 2);
        }

        for (int i = 3; i < segments * 2; i += 2) {
            indices.push_back(1);
            indices.push_back(i + 2);
            indices.push_back(i);
        }

        // Create final vertex array
        std::vector<Vertex> finalVertices;
        for (int index : indices) {
            finalVertices.push_back(vertices[index]);
        }

        return finalVertices;
    }

}
