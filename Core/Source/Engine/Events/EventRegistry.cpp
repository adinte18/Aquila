#include "Engine/Events/EventRegistry.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/EventBus.h"

#include "Engine/Renderer/Renderer.h"

#include "Platform/Filesystem/VirtualFileSystem.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneGraph.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"


namespace Engine {
    void EventRegistry::RegisterHandlers(Device* device, SceneManager* sceneManager, Renderer* renderer){
        Engine::EventBus::Get()->RegisterHandler<QueryEvent>([device, sceneManager](const QueryEvent& event){
            auto scene = sceneManager->GetActiveScene();
            if (!scene) return;
            
            switch(event.m_Command){

                case QueryCommand::EntityHasParent: {
                    auto& entityHandle = std::get<entt::entity>(event.m_Params.at("entity"));
                    auto& node = scene->GetRegistry().get<SceneNodeComponent>(entityHandle);

                    if (node.Parent != entt::null){
                        // has parent
                        event.m_Callback(UIEventResult::Success, {});
                    }
                }

                case QueryCommand::ExistingEntities:{
                    std::vector<AqEntity> entities;

                    auto view = scene->GetRegistry().view<SceneNodeComponent>();
                    for (auto entity : view) {
                        entities.push_back(AqEntity{entity, scene});
                    }

                    event.m_Callback(UIEventResult::Success, entities);
                    break;
                };
                case QueryCommand::GetAttachableEntities: {
                    auto& entity = std::get<entt::entity>(event.m_Params.at("entity"));
                    std::vector<Engine::AqEntity> attachableEntities;

                    auto& registry = scene->GetRegistry();
                    auto* sceneGraph = scene->GetSceneGraph();

                    auto isDescendant = [&](entt::entity ancestor, entt::entity descendant) -> bool {
                        return sceneGraph->IsDescendant(registry,ancestor, descendant);
                    };

                    auto isAncestor = [&](entt::entity descendant, entt::entity ancestor) -> bool {
                        return sceneGraph->IsDescendant(registry,ancestor, descendant);
                    };

                    for (auto e : registry.view<SceneNodeComponent>()) {
                        if (e == entity) continue;
                        if (isDescendant(entity, e)) continue;  // exclude descendants
                        if (isAncestor(entity, e)) continue;    // exclude ancestors

                        attachableEntities.emplace_back(Engine::AqEntity{ e, scene });
                    }

                    if (event.m_Callback)
                        event.m_Callback(UIEventResult::Success, attachableEntities);

                    break;
                }
                case QueryCommand::ContentBrowserAskTextures: {
                    std::vector<Ref<Engine::Texture2D>> textures;

                    //! todo : i dont like this, works for now but hardcoded 
                    std::vector<std::string> texturePaths = {
                        TEXTURES_PATH"/folder.png",
                        TEXTURES_PATH"/aquila-logo.png",
                        TEXTURES_PATH"/file.png"

                    };

                    for (const auto& path : texturePaths) {
                        Ref<Engine::Texture2D> texture = Engine::Texture2D::Builder(*device).setFilepath(path).build();
                        textures.push_back(texture);
                    }

                    if (event.m_Callback) {
                        event.m_Callback(UIEventResult::Success, textures);
                    }
                    
                    break;
                }
                case QueryCommand::EntityIsDescendant:{
                    auto& ancestor = std::get<entt::entity>(event.m_Params.at("ancestor"));
                    auto& descendant = std::get<entt::entity>(event.m_Params.at("descendant"));

                    bool found = false;

                    std::function<bool(entt::entity)> search = [&](entt::entity parent) -> bool {
                        if (!scene->GetRegistry().valid(parent)) return false;

                        auto* node = scene->GetRegistry().try_get<SceneNodeComponent>(parent);
                        if (!node) return false;

                        for (auto& child : node->Children) {
                            if (child == descendant) return true;
                            if (search(child)) return true;
                        }
                        return false;
                    };

                    found = search(ancestor);

                    if (event.m_Callback)
                        event.m_Callback(UIEventResult::Success, found);

                    break;
                }
                }
        });



        Engine::EventBus::Get()->RegisterHandler<UICommandEvent>([device, sceneManager, renderer](const UICommandEvent& event){
            auto scene = sceneManager->GetActiveScene();
            switch (event.m_Command) {
                case UICommand::ViewportResized: {
                    uint32_t width = std::get<int>(event.m_Params.at("Width"));
                    uint32_t height = std::get<int>(event.m_Params.at("Height"));

                    renderer->Resize({width, height});

                    break;
                }

                case UICommand::NewScene : {
                    sceneManager->EnqueueScene(CreateUnique<AquilaScene>("New Scene"));
                    sceneManager->RequestSceneChange();
                    break;
                }

                case UICommand::SaveScene: {
                    std::string baseName = scene->GetSceneName();
                    std::string virtualPath = "/Assets/" + baseName + ".aqscene";

                    int counter = 1;
                    while (VFS::VirtualFileSystem::Get()->Exists(virtualPath)) {
                        virtualPath = "/Assets/" + baseName + "_" + std::to_string(counter) + ".aqscene";
                        counter++;
                    }

                    scene->Serialize(virtualPath);
                    break;
                }

                case UICommand::OpenScene: {
                    auto& path = std::get<std::string>(event.m_Params.at("path"));
                    if (path.empty()) break;

                    std::cout << "Opening scene at : " << path << std::endl;

                    sceneManager->EnqueueScene(CreateUnique<AquilaScene>(),
                        [path](AquilaScene* scene) {
                            scene->Deserialize(path); // when the scene is activated, deserialize it
                        }
                    );

                    sceneManager->RequestSceneChange();
                    break;
                }

                case UICommand::AttachToEntity:{
                    auto& entity = std::get<entt::entity>(event.m_Params.at("entity"));
                    auto& parent = std::get<entt::entity>(event.m_Params.at("parent"));
                    
                    scene->GetSceneGraph()->AttachTo(scene->GetRegistry(), parent, entity);

                    break;
                }

                case UICommand::AddEntity: {
                    scene->GetEntityManager()->AddEntity();
                    break;
                }

                case UICommand::LoadMesh:{
                    auto& path = std::get<std::string>(event.m_Params.at("path"));
                    if (path.empty()) break;

                    if (event.m_Params.find("entity") == event.m_Params.end()) {
                        auto newEntity = scene->GetEntityManager()->AddEntity();
                        scene->GetRegistry().emplace<TransformComponent>(newEntity.GetHandle());
                        scene->GetRegistry().emplace<MeshComponent>(newEntity.GetHandle());

                        auto& meshData = scene->GetRegistry().get<MeshComponent>(newEntity.GetHandle());
                        meshData.data = CreateRef<Engine::Mesh>(*device);
                        meshData.data->Load(path);
                    }
                    else {
                        auto& entity = std::get<entt::entity>(event.m_Params.at("entity"));
                        if (!scene->GetRegistry().valid(entity)) {
                            Debug::LogError("Entity is not valid");
                            return;
                        }

                        auto& meshData = scene->GetRegistry().get<MeshComponent>(entity);
                        meshData.data = CreateRef<Engine::Mesh>(*device);
                        meshData.data->Load(path);
                    }

                    break;
                }
                case UICommand::CreateChildEntity:{
                    auto& entity = std::get<entt::entity>(event.m_Params.at("entity"));

                    scene->GetSceneGraph()->AddChild(scene->GetRegistry(), entity, scene->GetEntityManager()->AddEntity().GetHandle());

                    break;

                }
                case UICommand::DisownEntity:{
                    auto& entityHandle = std::get<entt::entity>(event.m_Params.at("entity"));
                    auto& node = scene->GetRegistry().get<SceneNodeComponent>(entityHandle);

                    if (node.Parent != entt::null){
                        scene->GetSceneGraph()->RemoveChild(scene->GetRegistry(), node.Parent, node.Entity);                    
                    }

                    break;
                }
                
                case UICommand::DeleteEntity:{
                    auto& entityHandle = std::get<entt::entity>(event.m_Params.at("entity"));
                    auto& node = scene->GetRegistry().get<SceneNodeComponent>(entityHandle);

                    if (!node.Children.empty()) 
                        scene->GetSceneGraph()->RemoveAllChildren(scene->GetRegistry(), node.Entity);

                    Engine::AqEntity{node.Entity, scene}.Kill();

                    break;
                }
                    
                case UICommand::AddPrimitiveCube:{
                    break;
                }
                case UICommand::AddPrimitiveSphere:{
                    break;
                }
                case UICommand::AddPrimitiveCylinder:{
                    break;
                }
                case UICommand::AddPrimitiveCapsule:{
                    break;
                }
            }
        });
    }
};