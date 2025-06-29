#ifndef SCENE_H
#define SCENE_H

#include "Components/Components.h"
#include "Entity.h"
#include <cstddef>
#include <memory>
#include "Engine/Buffer.h"
#include "Engine/Descriptor.h"
#include "Engine/OffscreenRenderer.h"
#include <glm/glm.hpp>
#include <queue>
#include <vector>

#include "Engine/EditorCamera.h"

namespace Engine {
    struct uboLight {
        alignas(4) int type;
        alignas(16) glm::vec3 color;
        alignas(4) float intensity;
        alignas(16) glm::vec3 direction;
    };

    struct UniformData {
        alignas(16) glm::mat4 projection{1.f};
        alignas(16) glm::mat4 view{1.f};
        alignas(16) glm::mat4 inverseView{1.f};
        alignas(16) uboLight light{};
        alignas(16) glm::mat4 lightSpaceMatrix{1.f};
    };

    struct MaterialData {
        alignas(16) glm::vec3 albedoColor {1.0f};
        alignas(4) float metallic = 0.0f;
        alignas(4) float roughness = 0.5f;
        alignas(16) glm::vec3 emissionColor {0.0f};
        alignas(4) float emissiveIntensity = 1.0f;
        alignas(4) float aoIntensity = 1.0f;
        alignas(8) glm::vec2 tiling {1.0f};
        alignas(8) glm::vec2 offset {0.0f};
        alignas(4) int invertNormalMap = 0;
    };


    class Scene {
    public:
        struct SceneNode {
            Ref<Entity> entity;

            Transform localTransform{}; // transform realtive to its parent

            glm::mat4 worldTransform{1.0f}; // absolute transform in the world

            WeakRef<SceneNode> parent;
            std::vector<Ref<SceneNode>> children;

            bool selected = false;

            void UpdateWorldTransform() {
                if (auto p = parent.lock()) {
                    worldTransform = p->worldTransform * localTransform.TransformMatrix();
                } else {
                    worldTransform = localTransform.TransformMatrix();
                }

                for (auto& child : children) {
                    child->UpdateWorldTransform();
                }
            }
        };


        Scene();
        ~Scene() = default;


        // TODO : old, deprecated, needs to be deleted
        Ref<Entity> CreateEntity();
        void DestroyEntity(const Entity& entity);
        void Clear();

        [[nodiscard]] entt::registry& GetRegistry();
        [[nodiscard]] Engine::EditorCamera& GetActiveCamera();
        [[nodiscard]] std::vector<Entity>& GetEntitesToDelete();
        void QueueForDestruction(entt::entity entity);
        void ClearQueue();

        void UseEnvMap(const bool value) { m_HasEnvMap = value; }
        [[nodiscard]] int HasEnvMap() const { return m_HasEnvMap; }


        // NEW : scene graph implementation
        Ref<SceneNode> CreateNode(Ref<SceneNode> parent = nullptr);

        // access to root node for rendering or traversal
        [[nodiscard]] const Ref<SceneNode>& GetRootNode() const { return m_RootNode; };
        Ref<SceneNode> GetSelectedNode(const Ref<SceneNode>& node);
        void DeselectAllNodes(const Ref<Engine::Scene::SceneNode>& node);
        const void TraverseNode(const Ref<SceneNode>& node);
        const void TraverseGraph();


    private:
        Ref<SceneNode> m_RootNode;
        
        Engine::EditorCamera m_Camera;
        entt::registry m_Registry;
        std::vector<Entity> m_QueuedForDestruction{};
        std::queue<entt::entity> recycledEntityIDs;
        int m_HasEnvMap;
    };
}



#endif
