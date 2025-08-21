#include "Scene/Scene.h"
#include "AquilaCore.h"
#include "Platform/Filesystem/VirtualFileSystem.h"
#include "Scene/Components/MetadataComponent.h"
#include "Scene/Components/SceneNodeComponent.h"
#include "Scene/EntityManager.h"
#include "Scene/SceneGraph.h"
#include "Engine/Controller.h"

namespace Engine {
    AquilaScene::AquilaScene() {
        m_SceneID = UUID::Generate();
    }

    AquilaScene::AquilaScene(const std::string& name)
        : m_SceneName(name) {
            m_SceneID = UUID::Generate();
    }

    AquilaScene::~AquilaScene() = default;

    
    /**
     * @brief Retrieves the entt registry associated with the scene.
     * 
     * @return entt::registry& The entt registry for the scene, which contains all entities and their components.
     */
    entt::registry& AquilaScene::GetRegistry() {
        return m_EntityManager->GetRegistry();
    }

    const UUID AquilaScene::GetHandle() const { return m_SceneID; }

    /**
     * @brief Retrieves the EntityManager associated with the scene.
     * 
     * @return EntityManager* Pointer to the EntityManager that manages entities in the scene.
     */
    EntityManager* AquilaScene::GetEntityManager(){
        return m_EntityManager.get();
    }

    /**
     * @brief Retrieves the SceneGraph associated with the scene.
     * 
     * @return SceneGraph* Pointer to the SceneGraph that manages the hierarchy and relationships of entities in the scene.
     */
    SceneGraph* AquilaScene::GetSceneGraph(){
        return m_SceneGraph.get();
    }

    /**
     * @brief Retrieves the name of the scene.
     * 
     * @return const std::string& The name of the scene.
     */
    const std::string& AquilaScene::GetSceneName(){
        return m_SceneName;
    }

    /**
     * @brief Initializes the scene by creating an EntityManager and a SceneGraph.
     * 
     * This function is called when the scene starts, setting up the necessary components
     * for managing entities and their relationships within the scene.
     */
    void AquilaScene::OnStart() {
        m_EntityManager = CreateUnique<EntityManager>(this);
        m_SceneGraph = CreateUnique<SceneGraph>();
        m_SceneGraph->Construct(m_EntityManager->GetRegistry());
    }

    /**
    * @brief Destroys the scene, cleaning up the EntityManager and SceneGraph.
    * 
    * This function is called when the scene is no longer needed, ensuring that all resources
    * associated with the scene are properly released.
    */
    bool AquilaScene::Deserialize(const std::string& filepath) {
        auto vfsFile = VFS::VirtualFileSystem::Get()->OpenFile(filepath, "r");

        if (!vfsFile->IsValid()) {
            return false;
        }

        std::vector<char> buffer(vfsFile->Size());
        size_t bytesRead = vfsFile->Read(buffer.data(), buffer.size());
        vfsFile->Close();

        nlohmann::ordered_json sceneJson;
        sceneJson = nlohmann::ordered_json::parse(buffer.begin(), buffer.end());
        
        m_SceneName = sceneJson.value("SceneName", "Untitled Scene");

        auto& registry = GetRegistry();
        registry.clear();

        if (!sceneJson.contains("Entities"))
            return false;

        const auto& entitiesJson = sceneJson["Entities"];

        std::unordered_map<std::string, entt::entity> uuidToEntity;

        for (auto& [idStr, entityData] : entitiesJson.items()) {
            entt::entity entity = registry.create();

            if (entityData.contains("MetadataComponent")) {
                const auto& meta = entityData["MetadataComponent"];
                MetadataComponent metadata;
                metadata.Name = meta.value("Name", "");
                metadata.ID = UUID::FromString(meta.value("UUID", ""));
                metadata.Enabled = meta.value("Enabled", true);
                registry.emplace<MetadataComponent>(entity, metadata);

                uuidToEntity[metadata.ID.ToString()] = entity;
            }
        }

        for (auto& [idStr, entityData] : entitiesJson.items()) {
            const auto& meta = entityData["MetadataComponent"];
            std::string uuidStr = meta.value("UUID", "");
            entt::entity entity = uuidToEntity.at(uuidStr);

            if (entityData.contains("TransformComponent")) {
                const auto& transformJson = entityData["TransformComponent"];
                glm::vec3 position = glm::vec3(
                    transformJson["Position"][0],
                    transformJson["Position"][1],
                    transformJson["Position"][2]);

                glm::vec3 rotation = glm::vec3(
                    transformJson["Rotation"][0],
                    transformJson["Rotation"][1],
                    transformJson["Rotation"][2]);

                glm::vec3 scale = glm::vec3(
                    transformJson["Scale"][0],
                    transformJson["Scale"][1],
                    transformJson["Scale"][2]);

                TransformComponent transform;
                transform.SetLocalPosition(position);
                transform.SetLocalRotation(rotation);
                transform.SetLocalScale(scale);
                transform.UpdateWorldMatrix();

                if (!registry.all_of<TransformComponent>(entity)) {
                    registry.emplace<TransformComponent>(entity, transform);
                } else {
                    registry.replace<TransformComponent>(entity, transform);
                }
            }

            if (entityData.contains("SceneNodeComponent")) {
                const auto& nodeJson = entityData["SceneNodeComponent"];
                SceneNodeComponent node;

                std::string parentUUID = nodeJson.value("Parent", "null");
                if (parentUUID == "null") {
                    node.Parent = entt::null;
                } else {
                    node.Parent = uuidToEntity.count(parentUUID) ? uuidToEntity.at(parentUUID) : entt::null;
                }

                if (nodeJson.contains("Children")) {
                    for (const auto& childUUIDJson : nodeJson["Children"]) {
                        std::string childUUID = childUUIDJson.get<std::string>();
                        if (uuidToEntity.count(childUUID)) {
                            node.Children.push_back(uuidToEntity.at(childUUID));
                        }
                    }
                }

                if (!registry.all_of<SceneNodeComponent>(entity)) {
                    registry.emplace<SceneNodeComponent>(entity, node);
                } else {
                    registry.replace<SceneNodeComponent>(entity, node);
                }
            }

            if (entityData.contains("MeshComponent")) {
                const auto& meshJson = entityData["MeshComponent"];
                MeshComponent mesh;
                mesh.data = CreateRef<Engine::Mesh>(Engine::Controller::Get()->GetDevice());
                mesh.data->Load(meshJson.value("Path", ""));
                
                if (!registry.all_of<MeshComponent>(entity)) {
                    registry.emplace<MeshComponent>(entity, mesh);
                } else {
                    registry.replace<MeshComponent>(entity, mesh);
                }
            }
        }

        return true;
    }

    /**
    * @brief Serializes the scene to a JSON file.
    * 
    * This function writes the current state of the scene, including entities and their components,
    * to a JSON file specified by the filepath.
    * 
    * @param filepath The path to the file where the scene will be serialized.
    * @return true if serialization was successful, false otherwise.
    */
    bool AquilaScene::Serialize(const std::string& filepath) {
        nlohmann::ordered_json sceneJson;

        sceneJson["SceneName"] = m_SceneName;

        auto& registry = GetRegistry();
        sceneJson["Entities"] = nlohmann::ordered_json::object();

        auto view = registry.view<entt::entity>();

        for (auto& entity : view) {
            nlohmann::ordered_json entityJson;

            auto& meta = registry.get<MetadataComponent>(entity);
            entityJson["MetadataComponent"] = {
                {"Name", meta.Name},
                {"UUID", meta.ID.ToString()},
                {"Enabled", meta.Enabled}
            };

            if (registry.all_of<TransformComponent>(entity)) {
                auto& transform = registry.get<TransformComponent>(entity);
                entityJson["TransformComponent"] = {
                    {"Position", { transform.GetLocalPosition().x, transform.GetLocalPosition().y, transform.GetLocalPosition().z }},
                    {"Rotation", { transform.GetLocalRotation().x, transform.GetLocalRotation().y, transform.GetLocalRotation().z }},
                    {"Scale", { transform.GetLocalScale().x, transform.GetLocalScale().y, transform.GetLocalScale().z }}
                };
            }

            if (registry.all_of<SceneNodeComponent>(entity)) {
                auto& node = registry.get<SceneNodeComponent>(entity);
                entityJson["SceneNodeComponent"] = {
                    {"Parent", node.Parent == entt::null ? "null" : registry.get<MetadataComponent>(node.Parent).ID.ToString()},
                    {"Children", nlohmann::ordered_json::array()}
                };

                for (auto& child : node.Children) {
                    entityJson["SceneNodeComponent"]["Children"].push_back(registry.get<MetadataComponent>(child).ID.ToString());
                }
            }

            if (registry.all_of<MeshComponent>(entity)) {
                auto& mesh = registry.get<MeshComponent>(entity);
                entityJson["MeshComponent"] = {
                    {"Path", mesh.data->GetPath()}
                };
            }

            sceneJson["Entities"][std::to_string(static_cast<int>(entity))] = entityJson;
        }

        auto vfsFile = VFS::VirtualFileSystem::Get()->OpenFile(filepath, "w");
        if (!vfsFile->IsValid()) {
            return false;
        }

        vfsFile->Write(sceneJson.dump(4).data(), sceneJson.dump(4).size());
        vfsFile->Close();

        return true;
    }


}