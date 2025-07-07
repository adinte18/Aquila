#include "Engine/Events/EventRegistry.h"
#include "Engine/Events/Event.h"
#include "Scene/SceneManager.h"

namespace Engine {
    void EventRegistry::RegisterHandlers(Device* device, Ref<SceneManager>& sceneManager, OffscreenRenderer* renderer){
        Engine::EventBus::Get().RegisterHandler<QueryEvent>([device, &sceneManager](const QueryEvent& event){
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



        Engine::EventBus::Get().RegisterHandler<UICommandEvent>([device, &sceneManager, renderer](const UICommandEvent& event){
            auto scene = sceneManager->GetActiveScene();
            switch (event.m_Command) {
                case UICommand::ViewportResized: {
                    uint32_t width = std::get<int>(event.m_Params.at("Width"));
                    uint32_t height = std::get<int>(event.m_Params.at("Height"));

                    renderer->Resize({width, height});

                    break;
                }

                case UICommand::NewScene : {
                    std::cout << "Creating new scene..." << std::endl;
                    sceneManager->EnqueueScene("NewScene", std::make_unique<AquilaScene>("NewScene"));
                    sceneManager->RequestSceneChange("NewScene");
                    break;
                }

                case UICommand::SaveScene: {
                    
                    std::string filename = std::string(ASSET_PATH"/" + scene->GetSceneName() + ".aqscene");
                    scene->Serialize(filename);

                    break;
                }

                case UICommand::OpenScene: {
                    auto& path = std::get<std::string>(event.m_Params.at("path"));
                    if (path.empty()) break;
                
                    sceneManager->EnqueueScene("OpenedScene", std::make_unique<AquilaScene>("OpenedScene"));
                    sceneManager->RequestSceneChange("OpenedScene");

                    // todo : deserialization should happen after the request is processed

                    break;
                }

                case UICommand::AttachToEntity:{
                    auto& entity = std::get<entt::entity>(event.m_Params.at("entity"));
                    auto& parent = std::get<entt::entity>(event.m_Params.at("parent"));
                    
                    scene->GetSceneGraph()->AttachTo(scene->GetRegistry(), parent, entity);

                    break;
                }

                case UICommand::AddEntity:
                    scene->GetEntityManager()->AddEntity();
                    break;

                case UICommand::LoadMesh:{
                    auto& path = std::get<std::string>(event.m_Params.at("path"));
                    auto& entity = std::get<entt::entity>(event.m_Params.at("entity"));

                    if (path.empty()) break;

                    auto& meshData = scene->GetRegistry().get<MeshComponent>(entity);
                    meshData.data = std::make_shared<Engine::Mesh>(*device);
                    meshData.data->Load(path);
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