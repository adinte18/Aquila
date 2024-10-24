//
// Created by alexa on 19/10/2024.
//

#include "UI.h"

#include <Scene.h>
#include <Engine/Model.h>
#include <nativefiledialog/src/nfd_common.h>

#include "Components.h"

void Editor::UI::OnStart() {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        if (vkCreateDescriptorPool(device.vk_GetDevice(), &pool_info, nullptr, &editorDescriptorPool) != VK_SUCCESS) 	{
            throw std::runtime_error("Cannot create descriptor pool for ImGui");
        }


        // Initialize ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\arial.ttf)", 16.0f);

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window.glfw_GetWindow(), true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = device.vk_GetInstance();
        init_info.PhysicalDevice = device.vk_GetPhysicalDevice();
        init_info.Device = device.vk_GetDevice();
        init_info.Queue = device.vk_GetGraphicsQueue();
        init_info.DescriptorPool = editorDescriptorPool;
        init_info.RenderPass = renderPass;
        init_info.Subpass = 0;
        init_info.MinImageCount = 3;
        init_info.ImageCount = 3;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = VK_NULL_HANDLE;
        ImGui_ImplVulkan_Init(&init_info);
}

void Editor::UI::OnUpdate(VkCommandBuffer commandBuffer, ECS::Scene& scene) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
    ImGui::PopStyleVar(2);

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open")) {

            }

            if (ImGui::MenuItem("Load")) {

                nfdchar_t *outPath = NULL;
                nfdresult_t result = NFD_OpenDialog( "gltf", NULL, &outPath );

                if (result == NFD_OKAY) {
                    std::cout << "Selected file: " << outPath << std::endl;

                    // Load model
                    std::shared_ptr<Engine::Model3D> model = Engine::Model3D::create(device);
                    model->Load(outPath, scene.GetMaterialDescriptorSetLayout(), scene.GetMaterialDescriptorPool());

                    // Create entity
                    auto entity = scene.CreateEntity();
                    entity->AddComponent<ECS::Mesh>();
                    entity->AddComponent<ECS::Transform>();

                    // If valid model, assign to entity
                    if (model != nullptr) {
                        entity->GetComponent<ECS::Mesh>().mesh = model;
                    }

                    NFDi_Free(outPath);
                } else if (result == NFD_CANCEL) {
                    std::cout << "User cancelled." << std::endl;
                } else {
                    std::cout << "Error: " << NFD_GetError() << std::endl;
                }
            }

            if (ImGui::MenuItem("Save")) {

            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit")) {

            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    if (ImGui::Begin("Asset manager")) {

        ImGui::End();
    }

    //Scene Hierarchy
    if (ImGui::Begin("Scene hierarchy", nullptr, ImGuiWindowFlags_NoScrollbar)) {
        // Iterate over all entities in the scene, and display them in the hierarchy.
        // If clicked, select the entity and show its properties

        // Iterate over all entities in the scene
        scene.GetRegistry().view<ECS::Transform>().each([&](auto entity, auto& transform) {
            ImGui::PushID((int)entity);

            bool isSelected = selectedEntity == entity;

            ECS::Entity ent = {scene.GetRegistry(), entity};

            if (ImGui::Selectable(ent.GetName().c_str(), isSelected)) {
                SetSelectedEntity(entity);
            }

            ImGui::PopID();
        });

        // Separator between hierarchy and properties panel
        ImGui::Separator();

        ImGui::End();
    }

    if (ImGui::Begin("Properties")) {
        // Display properties of the selected entity

        // If an entity is selected and is valid, display its properties
        if (GetSelectedEntity() != entt::null && scene.GetRegistry().valid(GetSelectedEntity())) {

            ImGui::Separator();

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup("PropertiesContextMenu");
            }

            if (ImGui::BeginPopup("PropertiesContextMenu")) {
                ECS::Entity ent = {scene.GetRegistry(), GetSelectedEntity()};

                if (ImGui::MenuItem("Add Mesh")) {
                    if (!ent.HasComponent<ECS::Mesh>()) {
                        ent.AddComponent<ECS::Mesh>();
                    }
                }

                if (ImGui::MenuItem("Add Transform")) {
                    if (!ent.HasComponent<ECS::Transform>()) {
                        ent.AddComponent<ECS::Transform>();
                    }
                }

                if (ImGui::MenuItem("Add Light")) {
                    if (!ent.HasComponent<ECS::Light>()) {
                        ent.AddComponent<ECS::Light>();
                    }
                }

                ImGui::EndPopup();
            }


            if (auto* transform = scene.GetRegistry().try_get<ECS::Transform>(GetSelectedEntity())) {
                ImGui::Text("Position");
                ImGui::DragFloat3("##Position", &transform->position.x, 0.1f);

                ImGui::Text("Rotation");
                ImGui::DragFloat3("##Rotation", &transform->rotation.x, 0.1f);

                ImGui::Text("Scale");
                ImGui::DragFloat3("##Scale", &transform->scale.x, 0.1f);
            }

            ImGui::Separator();

            if (auto* light = scene.GetRegistry().try_get<ECS::Light>(GetSelectedEntity())) {
                ImGui::Text("Light Type"); // Label
                ImGui::Combo("##LightType", (int*)&light->type, "Directional\0Point\0Spot\0\0");

                ImGui::Text("Color"); // Label
                ImGui::ColorEdit3("##ColorPicker", (float*)&light->color);

                ImGui::Text("Direction"); // Label
                ImGui::DragFloat3("##Direction", (float*)&light->direction, 0.1f);

                ImGui::Text("Intensity"); // Label
                ImGui::DragFloat("##Intensity", &light->intensity, 0.1f);
            }

            ImGui::Separator();

            if (auto* mesh = scene.GetRegistry().try_get<ECS::Mesh>(GetSelectedEntity())) {
                ImGui::Text("Current mesh: %s", mesh->mesh ? mesh->mesh->GetPath().c_str() : "None");

                // File input
                if (ImGui::Button("Load model")) {
                    nfdchar_t *outPath = NULL;
                    nfdresult_t result = NFD_OpenDialog( "gltf", NULL, &outPath );

                    if (result == NFD_OKAY) {
                        // Load texture
                        if (!mesh->mesh) {
                            std::cout << "Loading mesh" << std::endl;
                            auto loadedMesh = Engine::Model3D::create(device);
                            loadedMesh->Load(outPath, scene.GetMaterialDescriptorSetLayout(), scene.GetMaterialDescriptorPool());
                            mesh->mesh = std::move(loadedMesh);
                        }

                        NFDi_Free(outPath);
                    } else if (result == NFD_CANCEL) {
                        std::cout << "User cancelled." << std::endl;
                    } else {
                        std::cout << "Error: " << NFD_GetError() << std::endl;
                    }
                }
            }

        }

        ImGui::End();
    }

    //Viewport
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar)) {
        // If RMB is pressed, open context menu
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("ViewportContextMenu");
        }

        // Render the context menu if it has been opened
        if (ImGui::BeginPopup("ViewportContextMenu")) {
            if (ImGui::MenuItem("Create empty entity")) {
                auto entity = scene.CreateEntity();
                entity->AddComponent<ECS::Transform>();
                SetSelectedEntity(entity->GetHandle());
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Close")) {
                ImGui::CloseCurrentPopup();  // Close the context menu
            }
            ImGui::EndPopup();  // Close the popup
        }

        // Render scene to viewport
        ImVec2 viewportSize = ImGui::GetWindowSize();

        auto textureId = reinterpret_cast<ImTextureID>(scene.sceneView);
        ImGui::Image(textureId, { viewportSize.x, viewportSize.y });


        // Gizmos
        if (GetSelectedEntity() != entt::null && scene.GetRegistry().valid(GetSelectedEntity())) {
            ECS::Entity ent = {scene.GetRegistry(), GetSelectedEntity()};

            if (auto* transform = scene.GetRegistry().try_get<ECS::Transform>(GetSelectedEntity())) {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
                auto cameraView = glm::inverse(scene.GetActiveCamera().GetView());
                auto cameraProjection = scene.GetActiveCamera().GetProjection();
                auto transformComponent = transform->TransformMatrix();

                ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                     ImGuizmo::OPERATION::TRANSLATE, ImGuizmo::MODE::LOCAL, glm::value_ptr(transformComponent));

                if (ImGuizmo::IsUsing()) {
                    glm::vec3 translation, rotation, scale;
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transformComponent), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
                    transform->position = translation;
                    transform->rotation = glm::radians(rotation);  // Convert rotation from degrees to radians
                    transform->scale = scale;
                }
            }
        }


        ImGui::End();
    }
    ImGui::PopStyleVar();

    ImGui::End();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

}

