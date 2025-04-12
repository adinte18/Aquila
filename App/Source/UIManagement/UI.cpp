#include "UIManagement/UI.h"

#include <Scene.h>
#include <Engine/Framebuffer.h>
#include <Engine/Model.h>
#include <nativefiledialog/src/nfd_common.h>
#include <icons_font_awesome.h>

#include "Components.h"
#include "SceneContext.h"

void SetHazelImGuiTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Background colors
    colors[ImGuiCol_WindowBg]          = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ChildBg]            = ImVec4(0.14f, 0.14f, 0.14f, 1.0f); // Slightly brighter
    colors[ImGuiCol_PopupBg]            = ImVec4(0.12f, 0.12f, 0.12f, 0.98f); // Slightly brighter

    // Headers
    colors[ImGuiCol_Header]             = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // Slightly brighter
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.35f, 0.35f, 0.35f, 1.0f); // Slightly brighter
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.40f, 0.40f, 0.40f, 1.0f); // Slightly brighter

    // Borders
    colors[ImGuiCol_Border]             = ImVec4(0.30f, 0.30f, 0.30f, 1.0f); // Slightly brighter
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);

    // Frame BG
    colors[ImGuiCol_FrameBg]            = ImVec4(0.20f, 0.20f, 0.20f, 1.0f); // Slightly brighter
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // Slightly brighter
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.30f, 0.30f, 0.30f, 1.0f); // Slightly brighter

    // Buttons
    colors[ImGuiCol_Button]             = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.35f, 0.35f, 0.35f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.40f, 0.40f, 0.40f, 1.0f); // Slightly brighter

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f); // Slightly brighter

    // Tabs
    colors[ImGuiCol_Tab]                = ImVec4(0.20f, 0.20f, 0.20f, 1.0f); // Slightly brighter
    colors[ImGuiCol_TabHovered]         = ImVec4(0.30f, 0.30f, 0.30f, 1.0f); // Slightly brighter
    colors[ImGuiCol_TabActive]          = ImVec4(0.35f, 0.35f, 0.35f, 1.0f); // Slightly brighter
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.20f, 0.20f, 0.20f, 1.0f); // Slightly brighter
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // Slightly brighter

    // Title
    colors[ImGuiCol_TitleBg]            = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // Slightly brighter
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.20f, 0.20f, 0.20f, 1.0f); // Slightly brighter
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // Slightly brighter

    // Resizing grip
    colors[ImGuiCol_ResizeGrip]         = ImVec4(0.25f, 0.25f, 0.25f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.35f, 0.35f, 0.35f, 1.0f); // Slightly brighter
    colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.40f, 0.40f, 0.40f, 1.0f); // Slightly brighter

    // Separator
    colors[ImGuiCol_Separator]          = ImVec4(0.30f, 0.30f, 0.30f, 1.0f); // Slightly brighter
    colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.35f, 0.35f, 0.35f, 1.0f); // Slightly brighter
    colors[ImGuiCol_SeparatorActive]    = ImVec4(0.40f, 0.40f, 0.40f, 1.0f); // Slightly brighter

    // Text
    colors[ImGuiCol_Text]               = ImVec4(0.90f, 0.90f, 0.90f, 1.0f); // Slightly brighter
    colors[ImGuiCol_TextDisabled]       = ImVec4(0.55f, 0.55f, 0.55f, 1.0f); // Slightly brighter

    // Modify rounding for a modern look
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
}


void Editor::UIManager::InitializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    float baseFontSize = 20.0f;
    float iconFontSize = baseFontSize * 2.0f / 3.0f;

    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    icons_config.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\arial.ttf)", 14.0f);
    io.Fonts->AddFontFromFileTTF(R"(C:\Programming\Aquila\App\Include\fontawesome-webfont.ttf)", iconFontSize, &icons_config, icons_ranges);

    SetHazelImGuiTheme();
    ImGui::GetStyle().Alpha = 1.0f;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_Window.glfw_GetWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_Device.vk_GetInstance();
    init_info.PhysicalDevice = m_Device.vk_GetPhysicalDevice();
    init_info.Device = m_Device.vk_GetDevice();
    init_info.Queue = m_Device.vk_GetGraphicsQueue();
    init_info.DescriptorPool = m_UiDescriptorPool;
    init_info.RenderPass = m_RenderPass;
    init_info.Subpass = 0;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    ImGui_ImplVulkan_Init(&init_info);
}

void Editor::UIManager::OnStart() {
    // Descriptor pool for ImGui
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

    if (vkCreateDescriptorPool(m_Device.vk_GetDevice(), &pool_info, nullptr, &m_UiDescriptorPool) != VK_SUCCESS) 	{
        throw std::runtime_error("Cannot create descriptor pool for ImGui");
    }

    // Dear ImGui context
    InitializeImGui();
}

// void Editor::UIManager::Render(VkCommandBuffer commandBuffer) {
//     ImGui_ImplVulkan_NewFrame();
//     ImGui_ImplGlfw_NewFrame();
//     ImGui::NewFrame();
//     ImGuizmo::BeginFrame();
//
//     static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
//
//     ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
//     const ImGuiViewport* viewport = ImGui::GetMainViewport();
//     ImGui::SetNextWindowPos(viewport->WorkPos);
//     ImGui::SetNextWindowSize(viewport->WorkSize);
//     ImGui::SetNextWindowViewport(viewport->ID);
//     ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
//     ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
//     window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
//     window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
//
//     if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
//         window_flags |= ImGuiWindowFlags_NoBackground;
//     ImGui::PopStyleVar(2);
//
//     // Map to store the name buffer for each entity, keyed by entity ID
//     static std::unordered_map<entt::entity, std::string> nameBuffers;
//
//     ImGui::Begin("DockSpace Demo", nullptr, window_flags);
//
//     ImGuiIO& io = ImGui::GetIO();
//     if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
//     {
//         ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
//         ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
//     }
//
//
//     if (ImGui::Begin("Scene hierarchy", nullptr, ImGuiWindowFlags_NoScrollbar)) {
//         // If RMB is pressed, open context menu
//         if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
//             ImGui::OpenPopup("ViewportContextMenu");
//         }
//
//         // Render the context menu if it has been opened
//         if (ImGui::BeginPopup("ViewportContextMenu")) {
//             if (ImGui::MenuItem("Create empty entity")) {
//                 const auto entity = scene.CreateEntity();
//                 entity->AddComponent<ECS::Transform>();
//                 SetSelectedEntity(entity->GetHandle());
//             }
//             ImGui::Separator();
//
//             if (ImGui::BeginMenu("Primitives")) {
//
//                 if (ImGui::MenuItem("Create cube")) {
//                     auto entity = scene.CreateEntity();
//                     entity->AddComponent<ECS::Transform>();
//                     entity->AddComponent<ECS::PBRMaterial>();
//
//                     auto model = Engine::Model3D::create(device);
//                     model->CreatePrimitive(Engine::Primitives::PrimitiveType::Cube,
//                         1.0f,
//                         scene.GetMaterialDescriptorSetLayout(),
//                         scene.GetMaterialDescriptorPool());
//
//                     entity->AddComponent<ECS::Mesh>();
//                     entity->GetComponent<ECS::Mesh>().mesh = model;
//                     entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();
//
//                     SetSelectedEntity(entity->GetHandle());
//                 }
//
//
//                 if (ImGui::MenuItem("Create sphere")) {
//                     const auto entity = scene.CreateEntity();
//                     entity->AddComponent<ECS::Transform>();
//                     entity->AddComponent<ECS::PBRMaterial>();
//
//                     const auto model = Engine::Model3D::create(device);
//                     model->CreatePrimitive(Engine::Primitives::PrimitiveType::Sphere,
//                         1.0f,
//                         scene.GetMaterialDescriptorSetLayout(),
//                         scene.GetMaterialDescriptorPool());
//                     entity->AddComponent<ECS::Mesh>();
//                     entity->GetComponent<ECS::Mesh>().mesh = model;
//                     entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();
//
//                     SetSelectedEntity(entity->GetHandle());
//                 }
//
//                 ImGui::EndMenu();
//             }
//
//             if (ImGui::MenuItem("Create light")) {
//                 if (const auto entity = scene.CreateEntity()) {
//                     std::cout << "Entity created successfully." << std::endl;
//
//                     // Add the light component
//                     entity->AddComponent<ECS::Light>();
//                     std::cout << "Light component added." << std::endl;
//
//                     // Set the name of the entity
//                     entity->SetName("Directional light");
//                     std::cout << "Name set to: " << entity->GetName() << std::endl;  // Debug to verify the name is set
//
//                     nameBuffers[entity->GetHandle()] = entity->GetName();
//
//                     // Check if the Light component was added successfully
//                     if (entity->HasComponent<ECS::Light>()) {
//                         std::cout << "Light component added successfully." << std::endl;
//                     } else {
//                         std::cout << "Failed to add Light component." << std::endl;
//                     }
//
//                     // Select the newly created entity
//                     SetSelectedEntity(entity->GetHandle());
//                 } else {
//                     std::cout << "Failed to create entity." << std::endl;
//                 }
//             }
//
//             ImGui::Separator();
//
//
//             if (ImGui::MenuItem("Close")) {
//                 ImGui::CloseCurrentPopup();  // Close the context menu
//             }
//             ImGui::EndPopup();  // Close the popup
//         }
//
//         // Iterate over all entities in the scene, and display them in the hierarchy.
//         // If clicked, select the entity and show its properties
//
//         // Iterate over all entities in the scene
//         for (auto entity : scene.GetRegistry().view<entt::entity>()) {
//             ImGui::PushID(static_cast<int>(entity));
//
//             const bool isSelected = (selectedEntity == entity);
//             ECS::Entity ent = {scene.GetRegistry(), entity};
//
//             // Check if the entity's name is in nameBuffers, and display it if present
//             std::string displayName = ent.GetName(); // Default to entity's stored name
//             if (nameBuffers.find(entity) != nameBuffers.end()) {
//                 displayName = nameBuffers[entity];  // Use updated name if edited
//             }
//
//             if (ImGui::Selectable(displayName.c_str(), isSelected)) {
//                 SetSelectedEntity(entity);  // Update the selected entity when clicked
//             }
//
//             ImGui::PopID();
//         }
//         // Separator between hierarchy and properties panel
//         ImGui::Separator();
//
//         ImGui::End();
//     }
//
//
//     ImGui::End();
//     ImGui::End();
//     ImGui::Render();
//     ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
// }


void Editor::UIManager::OnEnd() {
    std::cout << "Destroying ImGui context" << std::endl;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    vkDestroyDescriptorPool(m_Device.vk_GetDevice(), m_UiDescriptorPool, nullptr);
    ImGui::DestroyContext();
}

void Editor::UIManager::UpdateDescriptorSets(ECS::SceneContext& sceneContext) {
    Engine::DescriptorWriter writer(sceneContext.GetMaterialDescriptorSetLayout(), sceneContext.GetMaterialDescriptorPool());

    auto& registry = m_Registry;

    for (auto entity : registry.view<ECS::PBRMaterial>()) {
        auto& material = registry.get<ECS::PBRMaterial>(entity);

        if (!material.dirty) {
            continue;
        }

        // Albedo Texture
        if (material.albedoTexture->isMarkedForDestruction) {
            material.albedoTexture->Destroy();
            material.albedoTexture.reset();
        }

        if (!material.albedoTexturePath.empty()) {
            material.albedoTexture = std::make_shared<Engine::Texture2D>(m_Device);
            material.albedoTexture->CreateTexture(material.albedoTexturePath);

            VkDescriptorImageInfo albedoImageInfo = material.albedoTexture->GetDescriptorSetInfo();
            writer.writeImage(0, &albedoImageInfo);
        }

        if (material.normalTexture->isMarkedForDestruction) {
            material.normalTexture->Destroy();
            material.normalTexture.reset();
        }

        if (!material.normalTexturePath.empty()) {
            material.normalTexture = std::make_shared<Engine::Texture2D>(m_Device);
            material.normalTexture->CreateTexture(material.normalTexturePath);

            VkDescriptorImageInfo normalImageInfo = material.normalTexture->GetDescriptorSetInfo();
            writer.writeImage(1, &normalImageInfo);
        }

        if (material.metallicRoughnessTexture->isMarkedForDestruction) {
            material.metallicRoughnessTexture->Destroy();
            material.metallicRoughnessTexture.reset();
        }

        if (!material.metallicRoughnessTexturePath.empty()) {
            material.metallicRoughnessTexture = std::make_shared<Engine::Texture2D>(m_Device);
            material.metallicRoughnessTexture->CreateTexture(material.metallicRoughnessTexturePath);

            VkDescriptorImageInfo metallicRoughnessImageInfo = material.metallicRoughnessTexture->GetDescriptorSetInfo();
            writer.writeImage(2, &metallicRoughnessImageInfo);
        }

        if (material.aoTexture->isMarkedForDestruction) {
            material.aoTexture->Destroy();
            material.aoTexture.reset();
        }

        if (!material.aoTexturePath.empty()) {
            material.aoTexture = std::make_shared<Engine::Texture2D>(m_Device);
            material.aoTexture->CreateTexture(material.aoTexturePath);

            VkDescriptorImageInfo aoImageInfo = material.aoTexture->GetDescriptorSetInfo();
            writer.writeImage(3, &aoImageInfo);
        }

        if (material.emissiveTexture->isMarkedForDestruction) {
            material.emissiveTexture->Destroy();
            material.emissiveTexture.reset();
        }

        if (!material.emissiveTexturePath.empty()) {
            material.emissiveTexture = std::make_shared<Engine::Texture2D>(m_Device);
            material.emissiveTexture->CreateTexture(material.emissiveTexturePath);

            VkDescriptorImageInfo emissiveImageInfo = material.emissiveTexture->GetDescriptorSetInfo();
            writer.writeImage(4, &emissiveImageInfo);
        }

        material.dirty = false;
        writer.overwrite(material.descriptorSet);
    }
}

void Editor::UIManager::GetFinalImage(VkDescriptorSet finalImage) {
    m_FinalImage = finalImage;
}

void Editor::UIManager::OnUpdate(VkCommandBuffer commandBuffer, ECS::SceneContext& sceneContext, float deltaTime) {
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

    if (ImGui::Begin("Testing window")) {

        if (ImGui::Button("Add entity")) {
            AddEntityEvent addEntityEvent;
            m_Dispatcher.Dispatch(addEntityEvent);
        }

        ImGui::End();
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
                    std::shared_ptr<Engine::Model3D> model = Engine::Model3D::create(m_Device);
                    model->Load(outPath, sceneContext.GetMaterialDescriptorSetLayout(), sceneContext.GetMaterialDescriptorPool());


                    // Create entity
                    const auto entity = sceneContext.GetScene().CreateEntity();
                    entity->AddComponent<ECS::Mesh>();
                    entity->AddComponent<ECS::Transform>();
                    entity->AddComponent<ECS::PBRMaterial>();

                    // If valid model, assign to entity
                    if (model != nullptr) {
                        entity->GetComponent<ECS::Mesh>().mesh = model;
                        entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();
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

    //Scene Hierarchy
    if (ImGui::Begin("Scene hierarchy", nullptr, ImGuiWindowFlags_NoScrollbar)) {
        // If RMB is pressed, open context menu
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup("ViewportContextMenu");
        }

        // Render the context menu if it has been opened
        if (ImGui::BeginPopup("ViewportContextMenu")) {
            if (ImGui::MenuItem("Create empty entity")) {
                const auto entity = sceneContext.GetScene().CreateEntity();
                entity->AddComponent<ECS::Transform>();
                SetSelectedEntity(entity->GetHandle());
            }
            ImGui::Separator();

            if (ImGui::BeginMenu("Primitives")) {

                if (ImGui::MenuItem("Create cube")) {
                    auto entity = sceneContext.GetScene().CreateEntity();
                    entity->AddComponent<ECS::Transform>();
                    entity->AddComponent<ECS::PBRMaterial>();

                    auto model = Engine::Model3D::create(m_Device);
                    model->CreatePrimitive(Engine::Primitives::PrimitiveType::Cube,
                        1.0f,
                        sceneContext.GetMaterialDescriptorSetLayout(),
                        sceneContext.GetMaterialDescriptorPool());

                    entity->AddComponent<ECS::Mesh>();
                    entity->GetComponent<ECS::Mesh>().mesh = model;
                    entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();

                    SetSelectedEntity(entity->GetHandle());
                }


                if (ImGui::MenuItem("Create sphere")) {
                    const auto entity = sceneContext.GetScene().CreateEntity();
                    entity->AddComponent<ECS::Transform>();
                    entity->AddComponent<ECS::PBRMaterial>();

                    const auto model = Engine::Model3D::create(m_Device);
                    model->CreatePrimitive(Engine::Primitives::PrimitiveType::Sphere,
                        1.0f,
                        sceneContext.GetMaterialDescriptorSetLayout(),
                        sceneContext.GetMaterialDescriptorPool());
                    entity->AddComponent<ECS::Mesh>();
                    entity->GetComponent<ECS::Mesh>().mesh = model;
                    entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();

                    SetSelectedEntity(entity->GetHandle());
                }

                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Create light")) {
                if (const auto entity = sceneContext.GetScene().CreateEntity()) {
                    std::cout << "Entity created successfully." << std::endl;

                    // Add the light component
                    entity->AddComponent<ECS::Light>();
                    std::cout << "Light component added." << std::endl;

                    // Set the name of the entity
                    entity->SetName("Directional light");
                    std::cout << "Name set to: " << entity->GetName() << std::endl;  // Debug to verify the name is set

                    nameBuffers[entity->GetHandle()] = entity->GetName();

                    // Check if the Light component was added successfully
                    if (entity->HasComponent<ECS::Light>()) {
                        std::cout << "Light component added successfully." << std::endl;
                    } else {
                        std::cout << "Failed to add Light component." << std::endl;
                    }

                    // Select the newly created entity
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
        for (auto entity : m_Registry.view<entt::entity>()) {
            ImGui::PushID(static_cast<int>(entity));

            const bool isSelected = (selectedEntity == entity);
            ECS::Entity ent = {m_Registry, entity};

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

if (ImGui::Begin("Environment map")) {

    if (ImGui::Button("Load environment map")) {
        nfdchar_t *outPath = nullptr;
        nfdresult_t result = NFD_OpenDialog( "hdr", nullptr, &outPath );
        if (result == NFD_OKAY) {
            std::cout << "Selected file: " << outPath << std::endl;

            AddEnvMapEvent addEnvMapEvent{outPath};
            m_Dispatcher.Dispatch(addEnvMapEvent);


            NFDi_Free(outPath);
        } else if (result == NFD_CANCEL) {
            std::cout << "User cancelled." << std::endl;
        } else {
            std::cout << "Error: " << NFD_GetError() << std::endl;
        }
    }

    ImGui::End();
}

if (ImGui::Begin("Properties")) {
    if (ImGui::Button("Reload shaders")) {
        m_ReloadShaders = true;
    }

    if (GetSelectedEntity() != entt::null && m_Registry.valid(GetSelectedEntity())) {
        ECS::Entity ent = {m_Registry, GetSelectedEntity()};
        entt::entity entityID = GetSelectedEntity();

        // Name Section
        if (nameBuffers.find(entityID) == nameBuffers.end()) {
            nameBuffers[entityID] = ent.GetName();
        }

        ImGui::Text("Name");
        ImGui::SameLine();
        std::string& nameBuffer = nameBuffers[entityID];
        char tempBuffer[128] = {0};
        nameBuffer.copy(tempBuffer, sizeof(tempBuffer) - 1);

        if (ImGui::InputText("##", tempBuffer, sizeof(tempBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
            nameBuffer = tempBuffer;
            ent.SetName(nameBuffer);
            std::cout << "Current entity name: " << ent.GetName() << std::endl;
        }

        // Add Component Button
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_PLUS)) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        ImGui::Separator();

        if (ImGui::BeginPopup("AddComponentPopup")) {
            if (ImGui::MenuItem("Mesh") && !ent.HasComponent<ECS::Mesh>()) {
                ent.AddComponent<ECS::Mesh>();
            }
            if (ImGui::MenuItem("Transform") && !ent.HasComponent<ECS::Transform>()) {
                ent.AddComponent<ECS::Transform>();
            }
            if (!ent.HasComponent<ECS::Light>() && !ent.HasComponent<ECS::Mesh>()) {
                if (ImGui::MenuItem("Light")) {
                    ent.AddComponent<ECS::Light>();
                }
            }
            ImGui::EndPopup();
        }

        // Transform Component
        if (auto* transform = m_Registry.try_get<ECS::Transform>(GetSelectedEntity())) {
            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                auto DrawVector3Control = [](const char* label, glm::vec3& values, float resetValue = 0.0f) {
                    ImGui::PushID(label);
                    ImGui::Columns(2);
                    ImGui::SetColumnWidth(0, 100);
                    ImGui::Text(label);
                    ImGui::NextColumn();

                    float buttonWidth = 20.0f;
                    float inputWidth = 70.0f;

                    const char* labels[] = { "X", "Y", "Z" };
                    ImVec4 colors[] = {
                        { 0.8f, 0.1f, 0.1f, 1.0f },
                        { 0.1f, 0.8f, 0.1f, 1.0f },
                        { 0.1f, 0.1f, 0.8f, 1.0f }
                    };

                    for (int i = 0; i < 3; i++) {
                        ImGui::PushStyleColor(ImGuiCol_Button, colors[i]);
                        if (ImGui::Button(labels[i], ImVec2(buttonWidth, 20))) values[i] = resetValue;
                        ImGui::PopStyleColor();
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(inputWidth);
                        ImGui::DragFloat(("##" + std::string(labels[i])).c_str(), &values[i], 0.1f, -FLT_MAX, FLT_MAX, "%.2f");
                        if (i < 2) ImGui::SameLine();
                    }

                    ImGui::Columns(1);
                    ImGui::PopID();
                };

                DrawVector3Control("Position", transform->position);
                glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(transform->rotation));
                DrawVector3Control("Rotation", eulerRotation);
                transform->rotation = glm::quat(glm::radians(eulerRotation));
                DrawVector3Control("Scale", transform->scale, 1.0f);
            }
        }

        // Mesh Component
        if (auto* mesh = m_Registry.try_get<ECS::Mesh>(GetSelectedEntity())) {
            if (ImGui::CollapsingHeader("Mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
                std::string meshName = mesh->mesh ? mesh->mesh->GetPath() : "Primitive";
                ImGui::Text("Current mesh: %s", meshName.c_str());

                if (ImGui::Button("Load Model")) {
                    nfdchar_t *outPath = nullptr;
                    nfdresult_t result = NFD_OpenDialog( "gltf", nullptr, &outPath );

                    if (result == NFD_OKAY) {
                        std::cout << "Selected file: " << outPath << std::endl;

                        // Load model
                        std::shared_ptr<Engine::Model3D> model = Engine::Model3D::create(m_Device);
                        model->Load(outPath, sceneContext.GetMaterialDescriptorSetLayout(), sceneContext.GetMaterialDescriptorPool());

                        // Create entity
                        const auto entity = sceneContext.GetScene().CreateEntity();
                        entity->AddComponent<ECS::Mesh>();
                        entity->AddComponent<ECS::Transform>();
                        entity->AddComponent<ECS::PBRMaterial>();

                        // If valid model, assign to entity
                        if (model != nullptr) {
                            entity->GetComponent<ECS::Mesh>().mesh = model;
                            entity->GetComponent<ECS::PBRMaterial>() = model->GetMaterial();
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

        // Material Component
if (auto* material = m_Registry.try_get<ECS::PBRMaterial>(GetSelectedEntity())) {
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Material Properties");

        ImGui::ColorEdit3("Albedo Color", reinterpret_cast<float*>(&material->albedoColor));

        if (material->albedoTexture) {
            ImGui::Text("Albedo Texture: ");
            ImGui::Image((ImTextureID)material->albedoTexture->GetDescriptorSet(), ImVec2(100, 100));  // Display the texture
        } else {
            ImGui::Text("No Albedo Texture");
        }
        if (ImGui::Button("Change Albedo Texture")) {
            // Open the file dialog
            nfdchar_t* outPath = nullptr;
            if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
                std::string filepath = outPath;
                material->albedoTexture->MarkForDestruction();
                material->albedoTexturePath = filepath;
                material->dirty = true;
            }
            free(outPath);
        }

        // Display and edit the metallic factor
        ImGui::SliderFloat("Metallic", &material->metallic, 0.0f, 1.0f);

        // Display the metallic-roughness texture and allow changing it
        if (material->metallicRoughnessTexture) {
            ImGui::Text("Metallic Roughness Texture: ");
            ImGui::Image((ImTextureID)material->metallicRoughnessTexture->GetDescriptorSet(), ImVec2(100, 100));
        } else {
            ImGui::Text("No Metallic Roughness Texture");
        }
        if (ImGui::Button("Change Metallic Roughness Texture")) {
            // Open the file dialog
            nfdchar_t* outPath = nullptr;
            if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
                std::string filepath = outPath;
                material->metallicRoughnessTexture->MarkForDestruction();
                material->metallicRoughnessTexturePath = filepath;
                material->dirty = true;
            }
            free(outPath);  // Free the path memory
        }

        // Display and edit the roughness factor
        ImGui::SliderFloat("Roughness", &material->roughness, 0.0f, 1.0f);

        // Display the normal map and allow changing it
        if (material->normalTexture) {
            ImGui::Text("Normal Texture: ");
            ImGui::Image((ImTextureID)material->normalTexture->GetDescriptorSet(), ImVec2(100, 100));
        } else {
            ImGui::Text("No Normal Texture");
        }

        if (ImGui::Button("Change Normal Texture")) {
            // Open the file dialog
            nfdchar_t* outPath = nullptr;
            if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
                std::string filepath = outPath;
                material->normalTexture->MarkForDestruction();
                material->normalTexturePath = filepath;
                material->dirty = true;
            }
            free(outPath);  // Free the path memory
        }

        ImGui::SameLine();
        ImGui::Checkbox("Invert Normal", reinterpret_cast<bool*>(&material->invertNormalMap));

        // Display and edit the emission color
        ImGui::ColorEdit3("Emission Color", reinterpret_cast<float*>(&material->emissionColor));

        // Display the emissive texture and allow changing it
        if (material->emissiveTexture) {
            ImGui::Text("Emissive Texture: ");
            ImGui::Image((ImTextureID)material->emissiveTexture->GetDescriptorSet(), ImVec2(100, 100));
        } else {
            ImGui::Text("No Emissive Texture");
        }
        if (ImGui::Button("Change Emissive Texture")) {
            // Open the file dialog
            nfdchar_t* outPath = nullptr;
            if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
                std::string filepath = outPath;
                material->emissiveTexture->MarkForDestruction();
                material->emissiveTexturePath = filepath;
                material->dirty = true;
            }
            free(outPath);  // Free the path memory
        }

        // Display and edit the AO (ambient occlusion) factor
        ImGui::SliderFloat("AO Intensity", &material->aoIntensity, 0.0f, 1.0f);

        // Display the AO texture and allow changing it
        if (material->aoTexture) {
            ImGui::Text("AO Texture: ");
            ImGui::Image((ImTextureID)material->aoTexture->GetDescriptorSet(), ImVec2(100, 100));
        } else {
            ImGui::Text("No AO Texture");
        }
        if (ImGui::Button("Change AO Texture")) {
            // Open the file dialog
            nfdchar_t* outPath = nullptr;
            if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
                std::string filepath = outPath;
                material->aoTexture->MarkForDestruction();
                material->aoTexturePath = filepath;
                material->dirty = true;
            }
            free(outPath);  // Free the path memory
        }

        // Display the emissive intensity slider
        ImGui::SliderFloat("Emissive Intensity", &material->emissiveIntensity, 0.0f, 10.0f);
    }
}


        // Light Component
        if (auto* light = m_Registry.try_get<ECS::Light>(GetSelectedEntity())) {
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Text("Light Type");
                ImGui::Combo("##LightType", reinterpret_cast<int*>(&light->type), "Directional\0Point\0Spot\0\0");

                ImGui::Text("Color");
                ImGui::ColorEdit3("##ColorPicker", reinterpret_cast<float*>(&light->color));

                ImGui::Text("Position");
                ImGui::DragFloat3("##Position", reinterpret_cast<float*>(&light->position), 0.1f);

                ImGui::Text("Intensity");
                ImGui::SliderFloat("##Intensity", &light->intensity, 0.0f, 10.0f);
            }
        }

        // Delete Entity
        ImGui::Separator();
        if (ImGui::Button("Delete Entity")) {
            ECS::Entity entityToDelete = {m_Registry, GetSelectedEntity()};
            sceneContext.GetScene().QueueForDestruction(entityToDelete.GetHandle());
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
                sceneContext.GetScene().GetActiveCamera().Rotate(deltaX, deltaY);

                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_W))) {
                    sceneContext.GetScene().GetActiveCamera().MoveForward(deltaTime);
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_S))) {
                    sceneContext.GetScene().GetActiveCamera().MoveBackward(deltaTime);
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_D))) {
                    sceneContext.GetScene().GetActiveCamera().MoveRight(deltaTime);
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A))) {
                    sceneContext.GetScene().GetActiveCamera().MoveLeft(deltaTime);
                }
                if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftShift))) {
                    sceneContext.GetScene().GetActiveCamera().SpeedUp();
                }
                else {
                    sceneContext.GetScene().GetActiveCamera().ResetSpeed();
                }
            }
        }

        static ImGuizmo::OPERATION m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE; // Default operation

        ImGui::SetNextWindowPos({ImGui::GetWindowPos().x + 10, ImGui::GetWindowPos().y + 30}, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(150, 50)); // Set the window size
        ImGui::Begin("Gizmo Controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        // Calculate the center position for the buttons
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 buttonSize(30, 30); // Assuming each button is 30x30 pixels
        float spacing = 10; // Space between buttons

        // Calculate the total width of the buttons including spacing
        float totalButtonsWidth = (buttonSize.x * 3) + (spacing * 2);

        // Calculate the starting X position for the first button
        float startX = (windowSize.x - totalButtonsWidth) / 2;

        // Calculate the starting Y position for the buttons
        float startY = (windowSize.y - buttonSize.y) / 2;

        // Set the cursor position to the center
        ImGui::SetCursorPos(ImVec2(startX, startY));

        if (ImGui::Button(ICON_FA_ARROWS)) {
            m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(startX + buttonSize.x + spacing);

        if (ImGui::Button(ICON_FA_EXPAND)) {
            m_CurrentOperation = ImGuizmo::OPERATION::SCALE;
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(startX + (buttonSize.x + spacing) * 2);

        if (ImGui::Button(ICON_FA_REPEAT)) {
            m_CurrentOperation = ImGuizmo::OPERATION::ROTATE;
        }

        ImGui::End();



        // Render scene to viewport
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();
        int newWidth = static_cast<int>(viewportSize.x);
        int newHeight = static_cast<int>(viewportSize.y);

        if (newWidth != lastViewportSize.x || newHeight != lastViewportSize.y) {
            m_ViewportResized = true;
            m_ViewportExtent = { static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y) };
            lastViewportSize = viewportSize;
        }

        auto textureId = reinterpret_cast<ImTextureID>(m_FinalImage);
        ImGui::Image(textureId, { viewportSize.x, viewportSize.y }, { 0, 1 }, { 1, 0 });

        // Gizmos
        if (GetSelectedEntity() != entt::null && m_Registry.valid(GetSelectedEntity())) {
            ECS::Entity ent = {m_Registry, GetSelectedEntity()};

            if (auto* transform = m_Registry.try_get<ECS::Transform>(GetSelectedEntity())) {
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
                auto cameraView = sceneContext.GetScene().GetActiveCamera().GetView();
                auto cameraProjection = sceneContext.GetScene().GetActiveCamera().GetProjection();
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
