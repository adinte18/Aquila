#include "Scene/Scene.h"
#include "AquilaCore.h"
#include "Scene/Components/MetadataComponent.h"
#include "Scene/EntityManager.h"
#include "Scene/SceneGraph.h"

namespace Engine {
    AquilaScene::AquilaScene(const std::string& name)
        : m_SceneName(name) {
    }

    AquilaScene::~AquilaScene() = default;

    entt::registry& AquilaScene::GetRegistry() {
        return m_EntityManager->GetRegistry();
    }

    EntityManager* AquilaScene::GetEntityManager(){
        return m_EntityManager.get();
    }

    SceneGraph* AquilaScene::GetSceneGraph(){
        return m_SceneGraph.get();
    }

    const std::string& AquilaScene::GetSceneName(){
        return m_SceneName;
    }

    void AquilaScene::OnStart() {
        m_EntityManager = std::make_unique<EntityManager>(this);
        m_SceneGraph = std::make_unique<SceneGraph>();
        m_SceneGraph->Construct(m_EntityManager->GetRegistry());
    }

    bool AquilaScene::Deserialize(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::ordered_json sceneJson;
        file >> sceneJson;
        file.close();

        m_SceneName = sceneJson.value("SceneName", "Untitled Scene");

        auto& registry = GetRegistry();

        if (!sceneJson.contains("Entities"))
            return false;

        const auto& entitiesJson = sceneJson["Entities"];

        for (auto& [idStr, entityData] : entitiesJson.items()) {
            entt::entity entity = registry.create();

            if (entityData.contains("MetadataComponent")) {
                const auto& meta = entityData["MetadataComponent"];
                MetadataComponent metadata;
                metadata.Name = meta.value("Name", "");
                metadata.ID = UUID::FromString( meta.value("UUID", ""));
                metadata.Enabled = meta.value("Enabled", true);
                registry.emplace<MetadataComponent>(entity, metadata);
            }

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
                registry.emplace<TransformComponent>(entity, transform);
            }
        }

        return true;
    }

    bool AquilaScene::Serialize(const std::string& filepath){
        nlohmann::ordered_json sceneJson;

        sceneJson["SceneName"] = m_SceneName;

        auto& registry = GetRegistry();

        sceneJson["Entities"] = nlohmann::ordered_json::object();

        auto view = registry.view<entt::entity>();

        for (auto& entity : view){
            nlohmann::ordered_json entityJson;

            auto& meta = registry.get<MetadataComponent>(entity);
            entityJson["MetadataComponent"] = {
                {"Name", {meta.Name}},
                {"UUID", {meta.ID.ToString()}},
                {"Enabled", {meta.Enabled}}
            };

            if (registry.all_of<TransformComponent>(entity)) {
                auto& transform = registry.get<TransformComponent>(entity);
                entityJson["TransformComponent"] = {
                    {"Position", { transform.GetLocalPosition().x, transform.GetLocalPosition().y, transform.GetLocalPosition().z }},
                    {"Rotation", { transform.GetLocalRotation().x, transform.GetLocalRotation().y, transform.GetLocalRotation().z }},
                    {"Scale", { transform.GetLocalScale().x, transform.GetLocalScale().y, transform.GetLocalScale().z }}
                };
            }

            sceneJson["Entities"][std::to_string(static_cast<int>(entity))] = entityJson;
        }

        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        file << sceneJson.dump(4);
        file.close();

        return true;
    }

}