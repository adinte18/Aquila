//
// Created by alexa on 19/10/2024.
//

#include "UI.h"

#include <Scene.h>
#include <Engine/Framebuffer.h>
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
        io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\arial.ttf)", 13.0f);

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

    // Map to store the name buffer for each entity, keyed by entity ID
    static std::unordered_map<entt::entity, std::string> nameBuffers;

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

                nfdchar_t *outPath = nullptr;
                nfdresult_t result = NFD_OpenDialog( "gltf", nullptr, &outPath );

                if (result == NFD_OKAY) {
                    std::cout << "Selected file: " << outPath << std::endl;

                    // Load model
                    std::shared_ptr<Engine::Model3D> model = Engine::Model3D::create(device);
                    model->Load(outPath, scene.GetMaterialDescriptorSetLayout(), scene.GetMaterialDescriptorPool());

                    // Create entity
                    const auto entity = scene.CreateEntity();
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
        // If RMB is pressed, open context menu
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("ViewportContextMenu");
        }

        // Render the context menu if it has been opened
        if (ImGui::BeginPopup("ViewportContextMenu")) {
            if (ImGui::MenuItem("Create empty entity")) {
                const auto entity = scene.CreateEntity();
                entity->AddComponent<ECS::Transform>();
                SetSelectedEntity(entity->GetHandle());
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Primitives")) {

                if (ImGui::MenuItem("Create cube")) {
                    auto entity = scene.CreateEntity();
                    entity->AddComponent<ECS::Transform>();

                    auto model = Engine::Model3D::create(device);
                    model->CreatePrimitive(Engine::Primitives::PrimitiveType::Cube,
                        1.0f,
                        scene.GetMaterialDescriptorSetLayout(),
                        scene.GetMaterialDescriptorPool());
                    entity->AddComponent<ECS::Mesh>();
                    entity->GetComponent<ECS::Mesh>().mesh = model;

                    SetSelectedEntity(entity->GetHandle());
                }


                if (ImGui::MenuItem("Create sphere")) {
                    const auto entity = scene.CreateEntity();
                    entity->AddComponent<ECS::Transform>();

                    const auto model = Engine::Model3D::create(device);
                    model->CreatePrimitive(Engine::Primitives::PrimitiveType::Sphere,
                        1.0f,
                        scene.GetMaterialDescriptorSetLayout(),
                        scene.GetMaterialDescriptorPool());
                    entity->AddComponent<ECS::Mesh>();
                    entity->GetComponent<ECS::Mesh>().mesh = model;

                    SetSelectedEntity(entity->GetHandle());
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Create light")) {
                if (const auto entity = scene.CreateEntity()) {
                    std::cout << "Entity created successfully." << std::endl;
                    entity->AddComponent<ECS::Light>();
                    if (entity->HasComponent<ECS::Light>()) {
                        std::cout << "Light component added successfully." << std::endl;
                    } else {
                        std::cout << "Failed to add Light component." << std::endl;
                    }
                    SetSelectedEntity(entity->GetHandle());
                } else {
                    std::cout << "Failed to create entity." << std::endl;
                }
            }

            ImGui::Separator();


            if (ImGui::MenuItem("Close")) {
                ImGui::CloseCurrentPopup();  // Close the context menu
            }
            ImGui::EndPopup();  // Close the popup
        }

        // Iterate over all entities in the scene, and display them in the hierarchy.
        // If clicked, select the entity and show its properties

        // Iterate over all entities in the scene
        for (auto entity : scene.GetRegistry().view<entt::entity>()) {
            ImGui::PushID(static_cast<int>(entity));

            const bool isSelected = (selectedEntity == entity);
            ECS::Entity ent = {scene.GetRegistry(), entity};

            // Check if the entity's name is in nameBuffers, and display it if present
            std::string displayName = ent.GetName(); // Default to entity's stored name
            if (nameBuffers.find(entity) != nameBuffers.end()) {
                displayName = nameBuffers[entity];  // Use updated name if edited
            }

            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                SetSelectedEntity(entity);  // Update the selected entity when clicked
            }

            ImGui::PopID();
        }
        // Separator between hierarchy and properties panel
        ImGui::Separator();

        ImGui::End();
    }

    if (ImGui::Begin("Properties")) {
        // Display properties of the selected entity

        // If an entity is selected and is valid, display its properties
        if (GetSelectedEntity() != entt::null && scene.GetRegistry().valid(GetSelectedEntity())) {
            ECS::Entity ent = {scene.GetRegistry(), GetSelectedEntity()};
            entt::entity entityID = GetSelectedEntity();

            if (nameBuffers.find(entityID) == nameBuffers.end()) {
                nameBuffers[entityID] = ent.GetName();
            }

            ImGui::Text("Name");
            ImGui::SameLine();

            std::string& nameBuffer = nameBuffers[entityID];
            char tempBuffer[128] = {0};  // Ensure null-termination

            // Use std::string::copy with the appropriate size
            nameBuffer.copy(tempBuffer, sizeof(tempBuffer) - 1);

            if (ImGui::InputText("##", tempBuffer, sizeof(tempBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                nameBuffer = tempBuffer;
                ent.SetName(nameBuffer);
                std::cout << "Current entity name: " << ent.GetName() << std::endl;
            }
            ImGui::Separator();

            if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            // Make sure the popup is displayed when triggered
            if (ImGui::BeginPopup("AddComponentPopup")) {
                ECS::Entity entity = {scene.GetRegistry(), GetSelectedEntity()};

                // Add menu items for different components
                if (ImGui::MenuItem("Mesh")) {
                    if (!entity.HasComponent<ECS::Mesh>()) {
                        entity.AddComponent<ECS::Mesh>();
                    }
                }
                if (ImGui::MenuItem("Transform")) {
                    if (!entity.HasComponent<ECS::Transform>()) {
                        entity.AddComponent<ECS::Transform>();
                    }
                }
                if (!entity.HasComponent<ECS::Light>() && !entity.HasComponent<ECS::Mesh>()) {
                    if (ImGui::MenuItem("Light")) {
                        entity.AddComponent<ECS::Light>();
                    }
                }

                // Close the popup after the menu items are displayed
                ImGui::EndPopup();
            }

            if (auto* transform = scene.GetRegistry().try_get<ECS::Transform>(GetSelectedEntity())) {
                ImGui::Text("Position");
                ImGui::DragFloat3("##Position", reinterpret_cast<float *>(&transform->position), 0.1f);

                ImGui::Text("Rotation");
                glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(transform->rotation));
                if (ImGui::DragFloat3("##Rotation", glm::value_ptr(eulerRotation), 0.5f)) {
                    transform->rotation = glm::quat(glm::radians(eulerRotation));
                }

                ImGui::Text("Scale");
                ImGui::DragFloat3("##Scale", reinterpret_cast<float *>(&transform->scale), 0.1f);
            }

            ImGui::Separator();

            if (auto* light = scene.GetRegistry().try_get<ECS::Light>(GetSelectedEntity())) {
                ImGui::Text("Light Type"); // Label
                ImGui::Combo("##LightType", reinterpret_cast<int *>(&light->type), "Directional\0Point\0Spot\0\0");

                ImGui::Text("Color"); // Label
                ImGui::ColorEdit3("##ColorPicker", reinterpret_cast<float *>(&light->color));

                ImGui::Text("Position"); // Label
                ImGui::DragFloat3("##Position", reinterpret_cast<float *>(&light->position), 0.1f);

                ImGui::Text("Intensity"); // Label
                ImGui::SliderFloat("##Intensity", &light->intensity, 0.f, 1.0f);

                light->UpdateMatrices();
            }

            ImGui::Separator();

            if (auto* mesh = scene.GetRegistry().try_get<ECS::Mesh>(GetSelectedEntity())) {
                ImGui::Text("Current mesh: %s", mesh->mesh ? mesh->mesh->GetPath().c_str() : "None");

                // File input
                if (ImGui::Button("Load model")) {
                    nfdchar_t *outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog("gltf", nullptr, &outPath);

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

            ImGui::Separator();

            if (ImGui::Button("Delete entity")) {
                ECS::Entity entityToDelete = {scene.GetRegistry(), GetSelectedEntity()};
                scene.queuedForDestruction.push_back(entityToDelete);
                SetSelectedEntity(entt::null);
            }
        }

        ImGui::End();
    }

    //Viewport
    //TODO: Refactor this, not really working as intended right now
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar)) {

        static bool isDragging = false;
        static ImVec2 lastMousePos;

        if (ImGui::IsWindowHovered()) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                isDragging = true;
                lastMousePos = ImGui::GetMousePos();
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                isDragging = false;
            }

            if (isDragging) {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 delta = {mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y};
                lastMousePos = mousePos;

                // Normalize delta
                float deltaX = delta.x / ImGui::GetWindowSize().x;
                float deltaY = delta.y / ImGui::GetWindowSize().y;

                // sets yaw and pitch
                scene.GetActiveCamera().Rotate(deltaX, deltaY);

                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_W))) {
                    scene.GetActiveCamera().MoveForward(scene.GetFrameTime());
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_S))) {
                    scene.GetActiveCamera().MoveBackward(scene.GetFrameTime());
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_D))) {
                    scene.GetActiveCamera().MoveRight(scene.GetFrameTime());
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A))) {
                    scene.GetActiveCamera().MoveLeft(scene.GetFrameTime());
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftShift))) {
                    scene.GetActiveCamera().SpeedUp();
                }
                else {
                    scene.GetActiveCamera().ResetSpeed();
                }
            }
        }

        static ImGuizmo::OPERATION m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE; // Default operation

        // Small window with icons on the top left
        ImGui::SetNextWindowPos({ImGui::GetWindowPos().x + 10, ImGui::GetWindowPos().y + 30}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(150, 50)); // Adjust the window size as needed
        ImGui::Begin("Gizmo Controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

            float windowWidth = ImGui::GetWindowSize().x;
            float windowHeight = ImGui::GetWindowSize().y;
            float buttonWidth = ImGui::CalcTextSize("T").x + ImGui::GetStyle().FramePadding.x * 2.0f;
            float buttonHeight = ImGui::GetFrameHeight();
            float totalWidth = buttonWidth * 3 + ImGui::GetStyle().ItemSpacing.x * 2.0f;
            float totalHeight = buttonHeight;

            ImGui::SetCursorPosX((windowWidth - totalWidth) / 2.0f);
            ImGui::SetCursorPosY((windowHeight - totalHeight) / 2.0f);

            if (ImGui::Button("T")) {
                m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE;
            }
            ImGui::SameLine();
            if (ImGui::Button("S")) {
                m_CurrentOperation = ImGuizmo::OPERATION::SCALE;
            }
            ImGui::SameLine();
            if (ImGui::Button("R")) {
                m_CurrentOperation = ImGuizmo::OPERATION::ROTATE;
            }

        ImGui::End();


        // Render scene to viewport
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        int newWidth = static_cast<int>(viewportSize.x);
        int newHeight = static_cast<int>(viewportSize.y);

        if (newWidth != lastViewportSize.x || newHeight != lastViewportSize.y) {
            viewportResized = true;
            viewportExtent = { static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y) };
            lastViewportSize = viewportSize;
        }

        auto textureId = reinterpret_cast<ImTextureID>(scene.sceneView);
        ImGui::Image(textureId, { viewportSize.x, viewportSize.y }, { 0, 1 }, { 1, 0 });

        // Gizmos
        if (GetSelectedEntity() != entt::null && scene.GetRegistry().valid(GetSelectedEntity())) {
            ECS::Entity ent = {scene.GetRegistry(), GetSelectedEntity()};

            if (auto* transform = scene.GetRegistry().try_get<ECS::Transform>(GetSelectedEntity())) {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
                auto cameraView = scene.GetActiveCamera().GetView();
                auto cameraProjection = scene.GetActiveCamera().GetProjection();
                auto transformComponent = transform->TransformMatrix();

                ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
                                     m_CurrentOperation, ImGuizmo::MODE::LOCAL, glm::value_ptr(transformComponent));

                if (ImGuizmo::IsUsing()) {

                    glm::vec3 translation, rotation, scale;
                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transformComponent),
                            glm::value_ptr(translation),
                            glm::value_ptr(rotation),
                            glm::value_ptr(scale));

                    transform->position = translation;
                    transform->scale = scale;
                    transform->rotation = glm::quat(glm::radians(rotation));
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

