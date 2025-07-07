
#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include "Engine/EditorCamera.h"
#include "UIManagement/UI.h"

#include "Engine/Window.h"
#include "Engine/Device.h"
#include "Engine/RenderManager.h"
#include "Engine/Texture2D.h"
#include "Engine/Events/EventRegistry.h"

#include "Scene/Scene.h"
#include "Scene/SceneManager.h"

#include "ECS/Scene.h"
#include "ECS/SceneContext.h"

#include "RenderingSystems/BRDFLutRenderingSystem.h"
#include "RenderingSystems/PrefilterRenderingSystem.h"
#include "RenderingSystems/CompositeRenderingSystem.h"
#include "RenderingSystems/EnvToCubemapRenderingSystem.h"
#include "RenderingSystems/SceneRenderingSystem.h"
#include "RenderingSystems/DepthRenderingSystem.h"
#include "RenderingSystems/GridRenderingSystem.h"
#include "RenderingSystems/CubemapRenderingSystem.h"
#include "RenderingSystems/PPRenderingSystem.h"
#include "RenderingSystems/IrradianceRenderingSystem.h"
#include "RenderingSystems/PreethamSkyRenderingSystem.h"
#include "RenderingSystems/SceneRenderingSystem_new.h"


namespace Editor {

    class EditorLayer {
    public:
        [[nodiscard]] Engine::Window& GetWindow() const { return *m_Window; }
        [[nodiscard]] Engine::Device& GetDevice() const { return *m_Device; }
        [[nodiscard]] Engine::RenderManager& GetRenderManager() const { return *m_RenderManager; }
        [[nodiscard]] UIManager& GetUIManager() const { return *m_UI; }

        EditorLayer() {
            OnStart();
        }

        ~EditorLayer() {
            OnEnd();
        }

        void OnStart();
        void OnUpdate();
        void OnEnd();

        

    private:
        // void RegisterHandlers() {
        //     EventBus::Get().RegisterHandler<UICommandEvent>([this](const UICommandEvent& e) {
        //         switch (e.m_Command) {
        //             case UICommand::AddEnvMap: {
        //                 auto& path = std::get<std::string>(e.m_Params.at("path"));
        //                 std::cout << "Loading texture from path: " << path << std::endl;
        //                 hdrUpdated = true;
        //                 m_HDRTexture = Engine::Texture2D::Builder(*m_Device)
        //                                 .setFilepath(path)
        //                                 .asHDR()
        //                                 .build();
        //
        //                 if (m_HDRTexture->HasImage() && m_HDRTexture->HasImageView()) {
        //                     e.m_Callback(UIEventResult::Success, m_HDRTexture);
        //                 }
        //                 else {
        //                     e.m_Callback(UIEventResult::Failure, {});
        //                 }
        //
        //                 break;
        //             }
        //             case UICommand::AddEntity: {
        //                 std::cout << "Adding a new entity..." << std::endl;
        //                 const auto node = m_SceneContext->GetScene().CreateNode();
        //                 const auto entity = m_Scene->CreateEntity();
        //                 entity->AddComponent<Engine::Transform>();
        //                 node->entity = entity;
        //
        //                 if (node) {
        //                     e.m_Callback(UIEventResult::Success, node);
        //                 }
        //                 else {
        //                     e.m_Callback(UIEventResult::Failure, {});
        //                 }
        //                 break;
        //             }
        //             case UICommand::AddLight: {
        //                 std::cout << "Adding a new light..." << std::endl;
        //                 const auto entity = m_Scene->CreateEntity();
        //                 entity->AddComponent<Engine::Transform>();
        //                 entity->AddComponent<Engine::Light>();
        //                 std::cout << "Light component added." << std::endl;
        //                 entity->SetName("Directional light");
        //
        //                 if (entity) {
        //                     e.m_Callback(UIEventResult::Success, entity);
        //                 }
        //                 else {
        //                     e.m_Callback(UIEventResult::Failure, {});
        //                 }
        //
        //                 break;
        //             }
        //             case UICommand::AddSphere : {
        //                 const auto entity = m_Scene->CreateEntity();
        //                 entity->AddComponent<Engine::Transform>();
        //                 entity->AddComponent<Engine::PBRMaterial>();
        //
        //                 const auto model = Engine::Model3D::create(*m_Device);
        //                 model->CreatePrimitive(Engine::Primitives::PrimitiveType::Sphere,
        //                     1.0f,
        //                     m_SceneContext->GetMaterialDescriptorSetLayout(),
        //                     m_SceneContext->GetMaterialDescriptorPool());
        //                 entity->AddComponent<Engine::Mesh>();
        //                 entity->GetComponent<Engine::Mesh>().mesh = model;
        //                 entity->GetComponent<Engine::PBRMaterial>() = model->GetMaterial();
        //
        //
        //                 if (entity) {
        //                     e.m_Callback(UIEventResult::Success, entity);
        //                 }
        //                 else {
        //                     e.m_Callback(UIEventResult::Failure, {});
        //                 }
        //                 break;
        //             }
        //
        //             case UICommand::AddMesh : {
        //                 //TODO : Fix this. this is not working as intended. I should get the currently selected entity and assign the mesh to its mesh component.
        //                 auto& path = std::get<std::string>(e.m_Params.at("path"));
        //                 std::cout << "Loading mesh from path: " << path << std::endl;
        //
        //                 Ref<Engine::Model3D> model = Engine::Model3D::create(*m_Device);
        //                 model->Load(path, m_SceneContext->GetMaterialDescriptorSetLayout(), m_SceneContext->GetMaterialDescriptorPool());
        //
        //                 const auto entity = m_Scene->CreateEntity();
        //                 entity->AddComponent<Engine::Mesh>();
        //                 entity->AddComponent<Engine::Transform>();
        //                 entity->AddComponent<Engine::PBRMaterial>();
        //
        //                 entity->GetComponent<Engine::Mesh>().mesh = model;
        //                 entity->GetComponent<Engine::PBRMaterial>() = model->GetMaterial();
        //
        //                 if (entity) {
        //                     e.m_Callback(UIEventResult::Success, entity);
        //                 }
        //                 else {
        //                     e.m_Callback(UIEventResult::Failure, {});
        //                 }
        //                 break;
        //             }
        //
        //             case UICommand::AddCube : {
        //                 Ref<Engine::Scene::SceneNode> node;
        //
        //                 auto& parentNode = std::get<Ref<Engine::Scene::SceneNode>>(e.m_Params.at("parent"));
        //                 if (parentNode){
        //                     node = m_SceneContext->GetScene().CreateNode(parentNode);
        //                 }
        //                 const auto entity = m_Scene->CreateEntity();
        //                 entity->AddComponent<Engine::Transform>();
        //                 entity->AddComponent<Engine::PBRMaterial>();
        //
        //                 const auto model = Engine::Model3D::create(*m_Device);
        //                 model->CreatePrimitive(Engine::Primitives::PrimitiveType::Cube,
        //                     1.0f,
        //                     m_SceneContext->GetMaterialDescriptorSetLayout(),
        //                     m_SceneContext->GetMaterialDescriptorPool());
        //                 entity->AddComponent<Engine::Mesh>();
        //                 entity->GetComponent<Engine::Mesh>().mesh = model;
        //                 entity->GetComponent<Engine::PBRMaterial>() = model->GetMaterial();
        //
        //                 node->entity = entity;
        //
        //                 if (node) {
        //                     e.m_Callback(UIEventResult::Success, node);
        //                 }
        //                 else {
        //                     e.m_Callback(UIEventResult::Failure, {});
        //                 }
        //                 break;
        //             }
        //
        //             case UICommand::RemoveEntity : {
        //                 auto& entity = std::get<Ref<Engine::Entity>>(e.m_Params.at("entity"));
        //                 m_Scene->QueueForDestruction(entity->GetHandle());
        //                 break;
        //             }
        //
        //             default:
        //                 break;
        //         }
        //     });
        // }

        void SampleIBL(VkCommandBuffer commandBuffer);
        void InitializeRenderingSystems();

        // Time
        float m_FrameTime{0.f};
        std::chrono::time_point<std::chrono::steady_clock> m_CurrentTime{};

        // testing
        bool hdrUpdated = false;
        Ref<Engine::Texture2D> m_HDRTexture;

        // Event registry
        Unique<Engine::EventRegistry> m_EventRegistry;

        // Window and device
        Unique<Engine::Window> m_Window;
        Unique<Engine::Device> m_Device;

        // Renderer manager (onscreen/offscreen)
        Unique<Engine::RenderManager> m_RenderManager;

        // Rendering systems
        Ref<Engine::SceneRenderingSystem> m_SceneRenderingSystem;
        Ref<Engine::DepthRenderingSystem> m_ShadowRenderingSystem;
        Ref<Engine::GridRenderingSystem> m_GridRenderingSystem;
        Ref<Engine::PPRenderingSystem> m_PostprocessingRenderingSystem;
        Ref<Engine::CompositeRenderingSystem> m_CompositeRenderingSystem;
        Ref<Engine::EnvToCubemapRenderingSystem> m_EnvToCubemapRenderingSystem;
        Ref<Engine::CubemapRenderingSystem> m_CubemapRenderingSystem;
        Ref<Engine::IrradianceRenderingSystem> m_IrradianceRenderingSystem;
        Ref<Engine::PrefilterRenderingSystem> m_PrefilterRenderingSystem;
        Ref<Engine::BRDFLutRenderingSystem> m_BRDFLutRenderingSystem;
        Ref<Engine::PreethamSkyRenderingSystem> m_PreethamSkyRenderingSystem;
        
        Ref<Engine::SceneRenderingSystem_new> m_SceneRendering;

        // UI Manager
        Unique<UIManager> m_UI;

        // Scene
        // Ref<Engine::Scene> m_Scene;
        Ref<Engine::SceneManager> m_SceneManager;
        Unique<Engine::EditorCamera> m_EditorCamera;


        // Scene context
        Unique<Engine::SceneContext> m_SceneContext;
    };
}



#endif //EDITORLAYER_H
