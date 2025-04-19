
#ifndef EDITORLAYER_H
#define EDITORLAYER_H
#include <memory>

#include <UIManagement/UI.h>

#include <Engine/Window.h>
#include <Engine/Device.h>
#include <Engine/RenderManager.h>

#include "SceneContext.h"
#include "Events/EventBus.h"
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
        void RegisterHandlers() {
            EventBus::Get().RegisterHandler<UICommandEvent>([this](const UICommandEvent& e) {
                switch (e.m_Command) {
                    case UICommand::AddEnvMap: {
                        auto& path = std::get<std::string>(e.m_Params.at("path"));
                        std::cout << "Loading texture from path: " << path << std::endl;
                        hdrUpdated = true;
                        m_HDRTexture = Engine::Texture2D::create(*m_Device);
                        m_HDRTexture->CreateHDRTexture(path);

                        if (m_HDRTexture->HasImage() && m_HDRTexture->HasImageView()) {
                            e.m_Callback(UIEventResult::Success, m_HDRTexture);
                        }
                        else {
                            e.m_Callback(UIEventResult::Failure, {});
                        }

                        break;
                    }
                    case UICommand::AddEntity: {
                        std::cout << "Adding a new entity..." << std::endl;
                        const auto entity = m_Scene->CreateEntity();
                        entity->AddComponent<ECS::Transform>();

                        if (entity) {
                            e.m_Callback(UIEventResult::Success, entity);
                        }
                        else {
                            e.m_Callback(UIEventResult::Failure, {});
                        }
                        break;
                    }
                    case UICommand::AddLight: {
                        std::cout << "Adding a new light..." << std::endl;
                        const auto entity = m_Scene->CreateEntity();
                        entity->AddComponent<ECS::Transform>();
                        entity->AddComponent<ECS::Light>();
                        std::cout << "Light component added." << std::endl;
                        entity->SetName("Directional light");

                        if (entity) {
                            e.m_Callback(UIEventResult::Success, entity);
                        }
                        else {
                            e.m_Callback(UIEventResult::Failure, {});
                        }

                        break;
                    }
                    case UICommand::AddSphere : {
                        const auto entity = m_Scene->CreateEntity();
                        entity->AddComponent<ECS::Transform>();
                        entity->AddComponent<ECS::PBRMaterial>();

                        const auto model = Engine::Model3D::create(*m_Device);
                        model->CreatePrimitive(Engine::Primitives::PrimitiveType::Sphere,
                            1.0f,
                            m_SceneContext->GetMaterialDescriptorSetLayout(),
                            m_SceneContext->GetMaterialDescriptorPool());
                        entity->AddComponent<ECS::Mesh>();
                        entity->GetComponent<ECS::Mesh>().mesh = model;
                        entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();


                        if (entity) {
                            e.m_Callback(UIEventResult::Success, entity);
                        }
                        else {
                            e.m_Callback(UIEventResult::Failure, {});
                        }
                        break;
                    }

                    case UICommand::AddMesh : {
                        auto& path = std::get<std::string>(e.m_Params.at("path"));
                        std::cout << "Loading texture from path: " << path << std::endl;

                        std::shared_ptr<Engine::Model3D> model = Engine::Model3D::create(*m_Device);
                        model->Load(path, m_SceneContext->GetMaterialDescriptorSetLayout(), m_SceneContext->GetMaterialDescriptorPool());

                        const auto entity = m_Scene->CreateEntity();
                        entity->AddComponent<ECS::Mesh>();
                        entity->AddComponent<ECS::Transform>();
                        entity->AddComponent<ECS::PBRMaterial>();

                        entity->GetComponent<ECS::Mesh>().mesh = model;
                        entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();

                        if (entity) {
                            e.m_Callback(UIEventResult::Success, entity);
                        }
                        else {
                            e.m_Callback(UIEventResult::Failure, {});
                        }
                        break;
                    }

                    case UICommand::AddCube : {
                        const auto entity = m_Scene->CreateEntity();
                        entity->AddComponent<ECS::Transform>();
                        entity->AddComponent<ECS::PBRMaterial>();

                        const auto model = Engine::Model3D::create(*m_Device);
                        model->CreatePrimitive(Engine::Primitives::PrimitiveType::Cube,
                            1.0f,
                            m_SceneContext->GetMaterialDescriptorSetLayout(),
                            m_SceneContext->GetMaterialDescriptorPool());
                        entity->AddComponent<ECS::Mesh>();
                        entity->GetComponent<ECS::Mesh>().mesh = model;
                        entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();


                        if (entity) {
                            e.m_Callback(UIEventResult::Success, entity);
                        }
                        else {
                            e.m_Callback(UIEventResult::Failure, {});
                        }
                        break;
                    }

                    case UICommand::RemoveEntity : {
                        auto& entity = std::get<std::shared_ptr<ECS::Entity>>(e.m_Params.at("entity"));
                        m_Scene->QueueForDestruction(entity->GetHandle());
                        break;
                    }

                    default:
                        break;
                }
            });
        }


        // Time
        float m_FrameTime{0.f};
        std::chrono::time_point<std::chrono::steady_clock> m_CurrentTime{};

        // testing
        bool hdrUpdated = false;
        std::shared_ptr<Engine::Texture2D> m_HDRTexture;

        // Window and device
        std::unique_ptr<Engine::Window> m_Window;
        std::unique_ptr<Engine::Device> m_Device;

        // Renderer manager (onscreen/offscreen)
        std::unique_ptr<Engine::RenderManager> m_RenderManager;

        // Rendering systems
        std::shared_ptr<RenderingSystem::SceneRenderingSystem> sceneRenderingSystem;
        std::shared_ptr<RenderingSystem::DepthRenderingSystem> shadowRenderingSystem;
        std::shared_ptr<RenderingSystem::GridRenderingSystem> gridRenderingSystem;
        std::shared_ptr<RenderingSystem::PPRenderingSystem> postprocessingRenderingSystem;
        std::shared_ptr<RenderingSystem::CompositeRenderingSystem> compositeRenderingSystem;
        std::shared_ptr<RenderingSystem::EnvToCubemapRenderingSystem> envToCubemapRenderingSystem;
        std::shared_ptr<RenderingSystem::CubemapRenderingSystem> cubemapRenderingSystem;
        std::shared_ptr<RenderingSystem::IrradianceRenderingSystem> irradianceRenderingSystem;
        std::shared_ptr<RenderingSystem::PrefilterRenderingSystem> prefilterRenderingSystem;
        std::shared_ptr<RenderingSystem::BRDFLutRenderingSystem> brdfLutRenderingSystem;

        // UI Manager
        std::unique_ptr<UIManager> m_UI;

        // Scene
        std::shared_ptr<ECS::Scene> m_Scene;

        // Scene context
        std::unique_ptr<ECS::SceneContext> m_SceneContext;

        std::unique_ptr<Engine::DescriptorPool> cubemapDescriptorPool{};
        std::unique_ptr<Engine::DescriptorSetLayout> cubemapDescriptorSetLayout{};
        VkDescriptorSet cubemapDescriptorSet;

    };
}



#endif //EDITORLAYER_H
