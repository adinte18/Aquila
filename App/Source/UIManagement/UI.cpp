#include "UIManagement/UI.h"
#include "Scene/Scene.h"

Editor::UIManager::Fonts Editor::UIManager::s_Fonts;
ImFont* Editor::UIManager::s_CurrentFont;
VkDescriptorSet Editor::UIManager::m_FinalImage = VK_NULL_HANDLE;
entt::entity Editor::UIManager::s_SelectedEntity = entt::null;

void Editor::UIManager::SetAquilaTheme() const
{
ImGuiStyle& style = ImGui::GetStyle();
ImVec4* colors = style.Colors;

    // Background colors
    colors[ImGuiCol_WindowBg]           = HEXAtoIV4("#1b1b1f", 1.0f);  // Dark background
    colors[ImGuiCol_ChildBg]            = HEXAtoIV4("#23272e", 1.0f);
    colors[ImGuiCol_PopupBg]            = HEXAtoIV4("#181a1f", 0.98f);

    colors[ImGuiCol_Header]             = HEXAtoIV4("#2f3136", 1.0f);
    colors[ImGuiCol_HeaderHovered]      = HEXAtoIV4("#393c43", 1.0f);
    colors[ImGuiCol_HeaderActive]       = HEXAtoIV4("#41454d", 1.0f);

    colors[ImGuiCol_Border]             = HEXAtoIV4("#2c2f33", 1.0f);
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);

    colors[ImGuiCol_FrameBg]            = HEXAtoIV4("#2f3136", 1.0f);
    colors[ImGuiCol_FrameBgHovered]     = HEXAtoIV4("#393c43", 1.0f);
    colors[ImGuiCol_FrameBgActive]      = HEXAtoIV4("#41454d", 1.0f);

    colors[ImGuiCol_Button]             = HEXAtoIV4("#4b5058", 1.0f);
    colors[ImGuiCol_ButtonHovered]      = HEXAtoIV4("#565c66", 1.0f);
    colors[ImGuiCol_ButtonActive]       = HEXAtoIV4("#2e333a", 1.0f);

    colors[ImGuiCol_ScrollbarBg]        = HEXAtoIV4("#1e2126", 1.0f);
    colors[ImGuiCol_ScrollbarGrab]      = HEXAtoIV4("#2f3136", 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = HEXAtoIV4("#393c43", 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = HEXAtoIV4("#41454d", 1.0f);

    colors[ImGuiCol_Tab]                = HEXAtoIV4("#2f3136", 1.0f);
    colors[ImGuiCol_TabHovered]         = HEXAtoIV4("#393c43", 1.0f);
    colors[ImGuiCol_TabActive]          = HEXAtoIV4("#41454d", 1.0f);
    colors[ImGuiCol_TabUnfocused]       = HEXAtoIV4("#2f3136", 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = HEXAtoIV4("#393c43", 1.0f);

    colors[ImGuiCol_TitleBg]            = HEXAtoIV4("#1b1b1f", 1.0f);
    colors[ImGuiCol_TitleBgActive]      = HEXAtoIV4("#23272e", 1.0f);
    colors[ImGuiCol_TitleBgCollapsed]   = HEXAtoIV4("#1b1b1f", 1.0f);

    colors[ImGuiCol_ResizeGrip]         = HEXAtoIV4("#2f3136", 1.0f);
    colors[ImGuiCol_ResizeGripHovered]  = HEXAtoIV4("#393c43", 1.0f);
    colors[ImGuiCol_ResizeGripActive]   = HEXAtoIV4("#41454d", 1.0f);

    colors[ImGuiCol_Separator]          = HEXAtoIV4("#2c2f33", 1.0f);
    colors[ImGuiCol_SeparatorHovered]   = HEXAtoIV4("#393c43", 1.0f);
    colors[ImGuiCol_SeparatorActive]    = HEXAtoIV4("#41454d", 1.0f);

    colors[ImGuiCol_Text]               = HEXAtoIV4("#eeeeee", 1.0f);
    colors[ImGuiCol_TextDisabled]       = HEXAtoIV4("#6c757d", 1.0f);

    style.WindowPadding = ImVec2(10, 10);
    style.WindowRounding = 0.f;
    style.TabRounding = 0.f;
    style.GrabRounding = 6.0f;
    style.ItemSpacing = ImVec2(10, 5);
    style.FramePadding.y = 2.0f;
    style.GrabMinSize = 10.0f;
    style.FrameRounding = 1.0f;
    style.WindowMenuButtonPosition = ImGuiDir_None;
}

ImVec4 Editor::UIManager::HEXAtoIV4(const char* hex, float alpha) const {
	int r, g, b;

    if (hex[0] == '#') hex++;

	sscanf_s(hex, "%02x%02x%02x", &r, &g, &b);
    return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, alpha);
}


void Editor::UIManager::LoadFont(ImGuiIO& io, Fonts& fonts, float size, ImFont*& outFont) const {
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 1;
    config.PixelSnapH = true;

    outFont = io.Fonts->AddFontFromFileTTF(UIConfig::MainFontPath, size, &config);
    
    static const ImWchar icon_ranges[] = { ICON_MIN_LC, ICON_MAX_16_LC, 0 };
    ImFontConfig icon_config;
    icon_config.MergeMode = true;
    icon_config.PixelSnapH = true;
    icon_config.GlyphMinAdvanceX = size;
    icon_config.GlyphOffset.y = size * 0.25f;

    io.Fonts->AddFontFromFileTTF(UIConfig::IconsFontPath, size, &icon_config, icon_ranges);
}


void Editor::UIManager::LoadFonts() const {
    ImGuiIO& io = ImGui::GetIO();

    LoadFont(io, s_Fonts, 12.0f, s_Fonts.Font12);
    LoadFont(io, s_Fonts, 14.0f, s_Fonts.Font14);
    LoadFont(io, s_Fonts, 16.0f, s_Fonts.Font16);
    LoadFont(io, s_Fonts, 18.0f, s_Fonts.Font18);
    LoadFont(io, s_Fonts, 20.0f, s_Fonts.Font20);
    LoadFont(io, s_Fonts, 24.0f, s_Fonts.Font24);
    LoadFont(io, s_Fonts, 28.0f, s_Fonts.Font28);
    LoadFont(io, s_Fonts, 32.0f, s_Fonts.Font32);
    LoadFont(io, s_Fonts, 40.0f, s_Fonts.Font40);
    LoadFont(io, s_Fonts, 48.0f, s_Fonts.Font48);
    LoadFont(io, s_Fonts, 64.0f, s_Fonts.Font64);
    LoadFont(io, s_Fonts, 72.0f, s_Fonts.Font72);

    io.Fonts->Build();
}

void Editor::UIManager::InitializeImGui() const {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= UIConfig::ConfigFlags;

    LoadFonts();

    SetAquilaTheme();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(m_Window.GetWindow(), true);
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

    s_CurrentFont = s_Fonts.Font16;
}

void Editor::UIManager::OnEnd() const {
    std::cout << "Destroying ImGui context" << std::endl;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    vkDestroyDescriptorPool(m_Device.vk_GetDevice(), m_UiDescriptorPool, nullptr);
    ImGui::DestroyContext();
}


void Editor::UIManager::GetFinalImage(VkDescriptorSet finalImage) {
    m_FinalImage = finalImage;
}

void Editor::UIManager::SetupDockspace(){
    ImGuiIO& io = ImGui::GetIO();
    
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    
    ImGui::Begin(UIConfig::DockspaceName, nullptr, UIConfig::WindowFlags);
    
    if (UIConfig::DockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
        UIConfig::WindowFlags |= ImGuiWindowFlags_NoBackground;


    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
        if (UIConfig::DockspaceID == 0)
            UIConfig::DockspaceID = ImGui::GetID(UIConfig::DockspaceName);
        
        ImGui::DockSpace(UIConfig::DockspaceID, ImVec2(0.0f, 0.0f), UIConfig::DockspaceFlags);
    }

}

void Editor::UIManager::OnUpdate(VkCommandBuffer commandBuffer, float deltaTime, Engine::AquilaScene* scene) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGuizmo::BeginFrame();

    if (s_CurrentFont) ImGui::PushFont(s_CurrentFont);


    SetupDockspace();

    //Draw menu bar
    m_Menubar.Draw(nullptr);

    //Draw panels
    m_ContentBrowser.Draw(nullptr);
    m_Hierarchy.Draw(scene);
    m_Properties.Draw(scene);
    m_Viewport.Draw(nullptr);

    if (s_CurrentFont) ImGui::PopFont();


    ImGui::End();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    ImGui::EndFrame();
}

// void Editor::UIManager::UpdateDescriptorSets(const Engine::SceneContext& sceneContext) const {
//     Engine::DescriptorWriter writer(sceneContext.GetMaterialDescriptorSetLayout(), sceneContext.GetMaterialDescriptorPool());

//     auto& registry = m_Registry;

//     for (auto entity : registry.view<Engine::PBRMaterial>()) {
//         auto& material = registry.get<Engine::PBRMaterial>(entity);

//         if (!material.dirty) {
//             continue;
//         }

//         // Albedo Texture
//         if (material.albedoTexture->isMarkedForDestruction) {
//             material.albedoTexture->Destroy();
//             material.albedoTexture.reset();
//         }

//         if (!material.albedoTexturePath.empty()) {
//             material.albedoTexture = std::make_shared<Engine::Texture2D>(m_Device);
//             material.albedoTexture->CreateTexture(material.albedoTexturePath);

//             VkDescriptorImageInfo albedoImageInfo = material.albedoTexture->GetDescriptorSetInfo();
//             writer.writeImage(0, &albedoImageInfo);
//         }

//         if (material.normalTexture->isMarkedForDestruction) {
//             material.normalTexture->Destroy();
//             material.normalTexture.reset();
//         }

//         if (!material.normalTexturePath.empty()) {
//             material.normalTexture = std::make_shared<Engine::Texture2D>(m_Device);
//             material.normalTexture->CreateTexture(material.normalTexturePath);

//             VkDescriptorImageInfo normalImageInfo = material.normalTexture->GetDescriptorSetInfo();
//             writer.writeImage(1, &normalImageInfo);
//         }

//         if (material.metallicRoughnessTexture->isMarkedForDestruction) {
//             material.metallicRoughnessTexture->Destroy();
//             material.metallicRoughnessTexture.reset();
//         }

//         if (!material.metallicRoughnessTexturePath.empty()) {
//             material.metallicRoughnessTexture = std::make_shared<Engine::Texture2D>(m_Device);
//             material.metallicRoughnessTexture->CreateTexture(material.metallicRoughnessTexturePath);

//             VkDescriptorImageInfo metallicRoughnessImageInfo = material.metallicRoughnessTexture->GetDescriptorSetInfo();
//             writer.writeImage(2, &metallicRoughnessImageInfo);
//         }

//         if (material.aoTexture->isMarkedForDestruction) {
//             material.aoTexture->Destroy();
//             material.aoTexture.reset();
//         }

//         if (!material.aoTexturePath.empty()) {
//             material.aoTexture = std::make_shared<Engine::Texture2D>(m_Device);
//             material.aoTexture->CreateTexture(material.aoTexturePath);

//             VkDescriptorImageInfo aoImageInfo = material.aoTexture->GetDescriptorSetInfo();
//             writer.writeImage(3, &aoImageInfo);
//         }

//         if (material.emissiveTexture->isMarkedForDestruction) {
//             material.emissiveTexture->Destroy();
//             material.emissiveTexture.reset();
//         }

//         if (!material.emissiveTexturePath.empty()) {
//             material.emissiveTexture = std::make_shared<Engine::Texture2D>(m_Device);
//             material.emissiveTexture->CreateTexture(material.emissiveTexturePath);

//             VkDescriptorImageInfo emissiveImageInfo = material.emissiveTexture->GetDescriptorSetInfo();
//             writer.writeImage(4, &emissiveImageInfo);
//         }

//         material.dirty = false;
//         writer.overwrite(material.descriptorSet);
//     }
// }
// void Editor::UIManager::OnUpdate(VkCommandBuffer commandBuffer, Engine::SceneContext& sceneContext, float deltaTime) {
//     ImGuiIO& io = ImGui::GetIO();


//     ImGui_ImplVulkan_NewFrame();
//     ImGui_ImplGlfw_NewFrame();
//     ImGui::NewFrame();
//     ImGuizmo::BeginFrame();
//     static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

//     ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
//     const ImGuiViewport* viewport = ImGui::GetMainViewport();

//     ImGui::SetNextWindowPos(viewport->WorkPos);
//     ImGui::SetNextWindowSize(viewport->WorkSize);
//     ImGui::SetNextWindowViewport(viewport->ID);

//     ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
//     ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

//     window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
//     window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

//     if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
//         window_flags |= ImGuiWindowFlags_NoBackground;
//     ImGui::PopStyleVar(2);

//     // Map to store the name buffer for each entity, keyed by entity ID
//     static std::unordered_map<entt::entity, std::string> nameBuffers;

//     ImGui::Begin("DockSpace Demo", nullptr, window_flags);

//     if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
//     {
//         m_DockspaceId = ImGui::GetID("MyDockSpace");
//         ImGui::DockSpace(m_DockspaceId, ImVec2(0.0f, 0.0f), dockspace_flags);
//     }

//     if (m_ShowSettings)
//     {
//         ImGui::SetNextWindowSize(ImVec2(1000, 800), ImGuiCond_Always);
//         ImGui::Begin("Settings", &m_ShowSettings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDocking);

//         auto& camera = sceneContext.GetScene().GetActiveCamera();
//         auto type = camera.GetType();
//         float childHeight = camera.GetType() == Engine::Camera::CameraType::Orbit ? 110.0f : 80.0f;
//         ImGui::SeparatorText(ICON_LC_VIDEO " CAMERA SETTINGS");

//         if (ImGui::BeginChild("ViewSection", ImVec2(0, childHeight), true)) {
//             ImGui::Text("View");
//             ImGui::Separator();

//             int cameraMode = type == Engine::Camera::CameraType::Orbit ? 1 : 0;
//             float btnWidth = 220.0f;

//             if (cameraMode == 0) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
//             if (ImGui::Button("Free", ImVec2(btnWidth, 0))) {
//                 camera.SwitchToType(Engine::Camera::CameraType::Free);
//             }
//             if (cameraMode == 0) ImGui::PopStyleColor();
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("Free camera: move and rotate freely.");

//             ImGui::SameLine();

//             if (cameraMode == 1) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
//             if (ImGui::Button("Orbit", ImVec2(btnWidth, 0))) {
//                 if (camera.OrbitAroundEntity()) {
//                     Engine::Entity ent = {m_Registry, GetSelectedEntity()};
//                     if (ent.HasComponent<Engine::Transform>()) {
//                         camera.SwitchToType(Engine::Camera::CameraType::Orbit, ent.GetComponent<Engine::Transform>().position);
//                     } else {
//                         camera.SwitchToType(Engine::Camera::CameraType::Orbit);
//                     }
//                 } else {
//                     camera.SwitchToType(Engine::Camera::CameraType::Orbit);
//                 }
//             }
//             if (cameraMode == 1) ImGui::PopStyleColor();
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("Orbit the camera around the selected entity.");


//             if (camera.GetType() == Engine::Camera::CameraType::Orbit) {
//                 ImGui::Checkbox("Orbit around currently selected entity", &camera.OrbitAroundEntity());
//             }

//             ImGui::EndChild();
//         }


//         // Movement and Control Section
//         if (ImGui::BeginChild("MovementControlSection", ImVec2(0, 180), true)) {
//             ImGui::Text("Movement and Control");
//             ImGui::Separator();

//             ImGui::PushItemWidth(-1);

//             // Rotation Sensitivity
//             ImGui::Text("Rotation Sensitivity");
//             ImGui::SameLine();
//             ImGui::TextDisabled(ICON_LC_INFO);
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("Controls how fast the camera rotates when moving the mouse.");
//             ImGui::SliderFloat("##rotSens", &camera.GetRotationSpeed(), 0.001f, 0.1f, "%.3f");

//             // Movement Speed
//             ImGui::Text("Movement Speed");
//             ImGui::SameLine();
//             ImGui::TextDisabled(ICON_LC_INFO);
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("Controls how fast the camera moves through the scene.");
//             ImGui::SliderFloat("##moveSpeed", &camera.GetMovementSpeed(), 0.1f, 20.0f, "%.2f");

//             ImGui::PopItemWidth();
//             ImGui::EndChild();
//         }

//         ImGui::Spacing();

//         if (ImGui::BeginChild("ProjectionSection", ImVec2(0, 200), true)) {
//             ImGui::Text("Projection");
//             ImGui::Separator();

//             bool projectionChanged = false;

//             float& fov = camera.GetFOV();
//             float& nearPlane = camera.GetNearPlane();
//             float& farPlane = camera.GetFarPlane();
//             float aspectRatio = camera.GetAspectRatio();

//             ImGui::Text("Projection Type");
//             ImGui::SameLine();
//             ImGui::TextDisabled(ICON_LC_INFO);
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("Choose between Perspective (3D) or Orthographic (2D-like) projection.");

//             ImGui::PushItemWidth(-1);

//             ImGui::Text("Field of View (FOV)");
//             ImGui::SameLine();
//             ImGui::TextDisabled(ICON_LC_INFO);
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("Controls the vertical field of view angle of the camera.");
//             if (ImGui::SliderFloat("##fov", &fov, 10.0f, 120.0f, "%.1f deg"))
//                 projectionChanged = true;

//             ImGui::Text("Near Plane");
//             ImGui::SameLine();
//             ImGui::TextDisabled(ICON_LC_INFO);
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("The closest distance from the camera where rendering starts.");
//             if (ImGui::SliderFloat("##near", &nearPlane, 0.01f, 10.0f, "%.2f"))
//                 projectionChanged = true;

//             ImGui::Text("Far Plane");
//             ImGui::SameLine();
//             ImGui::TextDisabled(ICON_LC_INFO);
//             if (ImGui::IsItemHovered())
//                 ImGui::SetTooltip("The farthest distance from the camera where rendering ends.");
//             if (ImGui::SliderFloat("##far", &farPlane, nearPlane + 1.0f, 1000.0f, "%.1f"))
//                 projectionChanged = true;

//             ImGui::PopItemWidth();

//             if (projectionChanged) {
//                 camera.SetPerspectiveProjection(fov, aspectRatio, nearPlane, farPlane);
//             }

//             ImGui::EndChild();
//         }

//         ImGui::Spacing();

//         ImGui::End();
//     }



//     if (ImGui::BeginMenuBar())
//     {
//         if (ImGui::BeginMenu("File"))
//         {
//             if (ImGui::MenuItem("Open")) {

//             }

//             if (ImGui::MenuItem("Load")) {

//                 nfdchar_t *outPath = nullptr;
//                 nfdresult_t result = NFD_OpenDialog( "gltf, fbx", nullptr, &outPath );

//                 if (result == NFD_OKAY) {
//                     std::cout << "Selected file: " << outPath << std::endl;

//                     EventCallback callback = [](const UIEventResult result, const CommandParam &payload) {
//                         if (result == UIEventResult::Success) {
//                             std::cout << "Mesh loaded successfully." << std::endl;
//                         } else {
//                             std::cout << "Failed to load mesh." << std::endl;
//                         }
//                     };

//                     EventBus::Get().Dispatch(UICommandEvent(
//                         UICommand::AddMesh,
//                         {{"path", std::string(outPath)}},
//                         callback
//                     ));

//                     NFDi_Free(outPath);
//                 } else if (result == NFD_CANCEL) {
//                     std::cout << "User cancelled." << std::endl;
//                 } else {
//                     std::cout << "Error: " << NFD_GetError() << std::endl;
//                 }
//             }

//             if (ImGui::MenuItem("Save")) {

//             }

//             ImGui::Separator();

//             if (ImGui::MenuItem("Exit")) {

//             }
//             ImGui::EndMenu();
//         }

//         if (ImGui::BeginMenu("Edit")) {
//             if (ImGui::MenuItem(ICON_LC_SETTINGS " Settings", nullptr, &m_ShowSettings)) {
//                 // boo
//             }

//             ImGui::EndMenu();
//         }

//         ImGui::EndMenuBar();
//     }

//     //Scene Hierarchy
//     if (ImGui::Begin(ICON_LC_LAYERS_2 " Scene hierarchy", nullptr, ImGuiWindowFlags_NoScrollbar)) {
//         // If RMB is pressed, open context menu
//         if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
//             ImGui::OpenPopup("ViewportContextMenu");
//         }

//         // Render the context menu if it has been opened
//         if (ImGui::BeginPopup("ViewportContextMenu")) {
//             if (ImGui::MenuItem("Create empty entity")) {
//                 EventCallback callback = [](const UIEventResult result, const CommandParam &payload) {
//                     if (result == UIEventResult::Success) {
//                         const auto entity = std::get<std::shared_ptr<Engine::Entity>>(payload);
//                         SetSelectedEntity(entity->GetHandle());
//                         std::cout << "Entity created successfully with ID: " << static_cast<int>(entity->GetHandle()) << std::endl;
//                     } else {
//                         std::cout << "Failed to create entity." << std::endl;
//                     }
//                 };

//                 EventBus::Get().Dispatch(UICommandEvent(UICommand::AddEntity, {}, callback));
//             }

//             if (ImGui::BeginMenu("Primitives")) {

//                 if (ImGui::MenuItem("Create cube")) {
//                     EventCallback callback = [](const UIEventResult result, const CommandParam &payload) {
//                         if (result == UIEventResult::Success) {
//                             const auto entity = std::get<std::shared_ptr<Engine::Entity>>(payload);
//                             nameBuffers[entity->GetHandle()] = entity->GetName();
//                             entity->SetName(std::string(ICON_LC_BOX + entity->GetName()));
//                             SetSelectedEntity(entity->GetHandle());
//                         } else {
//                             std::cout << "Failed to create entity." << std::endl;
//                         }
//                     };

//                     EventBus::Get().Dispatch(UICommandEvent(UICommand::AddCube, {}, callback));
//                 }


//                 if (ImGui::MenuItem("Create sphere")) {
//                     EventCallback callback = [](const UIEventResult result, const CommandParam &payload) {
//                         if (result == UIEventResult::Success) {
//                             const auto entity = std::get<std::shared_ptr<Engine::Entity>>(payload);
//                             nameBuffers[entity->GetHandle()] = entity->GetName();
//                             SetSelectedEntity(entity->GetHandle());
//                         } else {
//                             std::cout << "Failed to create entity." << std::endl;
//                         }
//                     };

//                     EventBus::Get().Dispatch(UICommandEvent(UICommand::AddSphere, {}, callback));
//                 }

//                 ImGui::EndMenu();
//             }

//             if (ImGui::MenuItem("Create light")) {
//                 EventCallback callback = [](const UIEventResult result, const CommandParam &payload) {
//                     if (result == UIEventResult::Success) {
//                         const auto entity = std::get<std::shared_ptr<Engine::Entity>>(payload);
//                         nameBuffers[entity->GetHandle()] = entity->GetName();
//                         SetSelectedEntity(entity->GetHandle());
//                     } else {
//                         std::cout << "Failed to create entity." << std::endl;
//                     }
//                 };

//                 EventBus::Get().Dispatch(UICommandEvent(UICommand::AddLight, {}, callback));
//             }

//             ImGui::Separator();


//             if (ImGui::MenuItem("Close")) {
//                 ImGui::CloseCurrentPopup();
//             }
//             ImGui::EndPopup();
//         }

//         for (auto entity : m_Registry.view<entt::entity>()) {
//             ImGui::PushID(static_cast<int>(entity));

//             const bool isSelected = (selectedEntity == entity);
//             Engine::Entity ent = {m_Registry, entity};

//             std::string displayName = ent.GetName();
//             if (nameBuffers.find(entity) != nameBuffers.end()) {
//                 displayName = nameBuffers[entity];
//             }

//             if (ImGui::Selectable(displayName.c_str(), isSelected)) {
//                 SetSelectedEntity(entity);
//             }

//             ImGui::PopID();
//         }
//     }
//     ImGui::End();

//     if (ImGui::Begin(ICON_LC_LEAF " Environment map")) {

//         if (ImGui::Button("Load environment map")) {
//             nfdchar_t *outPath = nullptr;
//             nfdresult_t result = NFD_OpenDialog("hdr", nullptr, &outPath);
//             if (result == NFD_OKAY) {
//                 std::cout << "Selected file: " << outPath << std::endl;

//                 EventCallback callback = [](const UIEventResult result, const CommandParam &payload) {
//                     if (result == UIEventResult::Success) {
//                         std::cout << "Environment map loaded successfully." << std::endl;
//                     } else {
//                         std::cout << "Failed to load environment map." << std::endl;
//                     }
//                 };

//                 EventBus::Get().Dispatch(UICommandEvent(
//                     UICommand::AddEnvMap,
//                     {{"path", std::string(outPath)}},
//                     callback
//                 ));

//                 NFDi_Free(outPath);
//             } else if (result == NFD_CANCEL) {
//                 std::cout << "User cancelled." << std::endl;
//             } else {
//                 std::cout << "Error: " << NFD_GetError() << std::endl;
//             }
//         }
//     }
//     ImGui::End();


//     ImGui::Begin(ICON_LC_SLIDERS_HORIZONTAL " Properties");
//     {
//         if (auto node = sceneContext.GetScene().GetSelectedNode(sceneContext.GetScene().GetRootNode())){
//             if (auto* transform = m_Registry.try_get<Engine::Transform>(node->entity->GetHandle())) {
//                 if (ImGui::CollapsingHeader(ICON_LC_MOVE_3D " TRANSFORM", ImGuiTreeNodeFlags_DefaultOpen)) {
//                     auto DrawVector3Control = [](const char* label, glm::vec3& values, float resetValue = 0.0f) {
//                         ImGui::PushID(label);
//                         ImGui::Columns(2);
//                         ImGui::SetColumnWidth(0, 100);
//                         ImGui::Text(label);
//                         ImGui::NextColumn();

//                         const char* labels[] = { "X", "Y", "Z" };
//                         ImVec4 colors[] = {
//                             { 0.8f, 0.1f, 0.1f, 1.0f },
//                             { 0.1f, 0.8f, 0.1f, 1.0f },
//                             { 0.1f, 0.1f, 0.8f, 1.0f }
//                         };

//                         for (int i = 0; i < 3; i++) {
//                             constexpr float inputWidth = 70.0f;
//                             ImGui::PushStyleColor(ImGuiCol_Button, colors[i]);
//                             if (constexpr float buttonWidth = 20.0f; ImGui::Button(labels[i], ImVec2(buttonWidth, 20))) values[i] = resetValue;
//                             ImGui::PopStyleColor();
//                             ImGui::SameLine();
//                             ImGui::SetNextItemWidth(inputWidth);
//                             ImGui::DragFloat(("##" + std::string(labels[i])).c_str(), &values[i], 0.1f, -FLT_MAX, FLT_MAX, "%.2f");
//                             if (i < 2) ImGui::SameLine();
//                         }

//                         ImGui::Columns(1);
//                         ImGui::PopID();
//                     };

//                     DrawVector3Control("Position", transform->position);
//                     glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(transform->rotation));
//                     DrawVector3Control("Rotation", eulerRotation);
//                     transform->rotation = glm::quat(glm::radians(eulerRotation));
//                     DrawVector3Control("Scale", transform->scale, 1.0f);
//                 }
//             }
//         }


//         if (GetSelectedEntity() != entt::null && m_Registry.valid(GetSelectedEntity())) {
//             Engine::Entity ent = {m_Registry, GetSelectedEntity()};
//             entt::entity entityID = GetSelectedEntity();

//             if (nameBuffers.find(entityID) == nameBuffers.end()) {
//                 nameBuffers[entityID] = ent.GetName();
//             }

//             ImGui::Text("Name");
//             ImGui::SameLine();
//             std::string& nameBuffer = nameBuffers[entityID];
//             char tempBuffer[128] = {0};
//             nameBuffer.copy(tempBuffer, sizeof(tempBuffer) - 1);

//             if (ImGui::InputText("##", tempBuffer, sizeof(tempBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
//                 nameBuffer = tempBuffer;
//                 ent.SetName(nameBuffer);
//                 std::cout << "Current entity name: " << ent.GetName() << std::endl;
//             }

//             // Add Component Button
//             ImGui::SameLine();
//             if (ImGui::Button(ICON_LC_PLUS)) {
//                 ImGui::OpenPopup("AddComponentPopup");
//             }

//             ImGui::SameLine();
//             if (ImGui::Button(ICON_LC_TRASH_2)) {
//                 // Create a shared pointer to the entity
//                 std::shared_ptr<Engine::Entity> entityToDelete = std::make_shared<Engine::Entity>(m_Registry, GetSelectedEntity());

//                 entt::entity entityHandle = entityToDelete->GetHandle();

//                 if (m_Registry.valid(entityHandle)) {
//                     nameBuffers.erase(entityHandle);

//                     SetSelectedEntity(entt::null);

//                     EventBus::Get().Dispatch(UICommandEvent(
//                         UICommand::RemoveEntity,
//                         {{"entity", entityToDelete}},
//                         nullptr
//                     ));
//                 }
//             }

//             ImGui::Separator();

//             if (ImGui::BeginPopup("AddComponentPopup")) {
//                 if (ImGui::MenuItem("Mesh") && !ent.HasComponent<Engine::Mesh>()) {
//                     ent.AddComponent<Engine::Mesh>();
//                 }
//                 if (ImGui::MenuItem("Transform") && !ent.HasComponent<Engine::Transform>()) {
//                     ent.AddComponent<Engine::Transform>();
//                 }
//                 if (!ent.HasComponent<Engine::Light>() && !ent.HasComponent<Engine::Mesh>()) {
//                     if (ImGui::MenuItem("Light")) {
//                         ent.AddComponent<Engine::Light>();
//                     }
//                 }
//                 ImGui::EndPopup();
//             }

//             // Transform Component
//             if (auto* transform = m_Registry.try_get<Engine::Transform>(GetSelectedEntity())) {
//                 if (ImGui::CollapsingHeader(ICON_LC_MOVE_3D " TRANSFORM", ImGuiTreeNodeFlags_DefaultOpen)) {
//                     auto DrawVector3Control = [](const char* label, glm::vec3& values, float resetValue = 0.0f) {
//                         ImGui::PushID(label);
//                         ImGui::Columns(2);
//                         ImGui::SetColumnWidth(0, 100);
//                         ImGui::Text(label);
//                         ImGui::NextColumn();

//                         const char* labels[] = { "X", "Y", "Z" };
//                         ImVec4 colors[] = {
//                             { 0.8f, 0.1f, 0.1f, 1.0f },
//                             { 0.1f, 0.8f, 0.1f, 1.0f },
//                             { 0.1f, 0.1f, 0.8f, 1.0f }
//                         };

//                         for (int i = 0; i < 3; i++) {
//                             constexpr float inputWidth = 70.0f;
//                             ImGui::PushStyleColor(ImGuiCol_Button, colors[i]);
//                             if (constexpr float buttonWidth = 20.0f; ImGui::Button(labels[i], ImVec2(buttonWidth, 20))) values[i] = resetValue;
//                             ImGui::PopStyleColor();
//                             ImGui::SameLine();
//                             ImGui::SetNextItemWidth(inputWidth);
//                             ImGui::DragFloat(("##" + std::string(labels[i])).c_str(), &values[i], 0.1f, -FLT_MAX, FLT_MAX, "%.2f");
//                             if (i < 2) ImGui::SameLine();
//                         }

//                         ImGui::Columns(1);
//                         ImGui::PopID();
//                     };

//                     DrawVector3Control("Position", transform->position);
//                     glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(transform->rotation));
//                     DrawVector3Control("Rotation", eulerRotation);
//                     transform->rotation = glm::quat(glm::radians(eulerRotation));
//                     DrawVector3Control("Scale", transform->scale, 1.0f);
//                 }
//             }

//             // Mesh Component
//             if (auto* mesh = m_Registry.try_get<Engine::Mesh>(GetSelectedEntity())) {
//                 if (ImGui::CollapsingHeader(ICON_LC_BOX " MESH", ImGuiTreeNodeFlags_DefaultOpen)) {
//                     std::string meshName = mesh->mesh ? mesh->mesh->GetPath() : "Primitive";
//                     ImGui::Text("Current mesh: %s", meshName.c_str());

//                     if (ImGui::Button("Load Model")) {
//                         nfdchar_t *outPath = nullptr;
//                         nfdresult_t result = NFD_OpenDialog( "gltf", nullptr, &outPath );

//                         if (result == NFD_OKAY) {
//                             std::cout << "Selected file: " << outPath << std::endl;

//                             EventCallback callback = [](const UIEventResult result, const CommandParam &payload) {
//                                 if (result == UIEventResult::Success) {
//                                     std::cout << "Mesh loaded successfully." << std::endl;
//                                 } else {
//                                     std::cout << "Failed to load mesh." << std::endl;
//                                 }
//                             };

//                             EventBus::Get().Dispatch(UICommandEvent(
//                                 UICommand::AddMesh,
//                                 {{"path", std::string(outPath)}},
//                                 callback
//                             ));

//                             NFDi_Free(outPath);
//                         } else if (result == NFD_CANCEL) {
//                             std::cout << "User cancelled." << std::endl;
//                         } else {
//                             std::cout << "Error: " << NFD_GetError() << std::endl;
//                         }
//                     }
//                 }
//             }

//             // Material Component
//         if (auto* material = m_Registry.try_get<Engine::PBRMaterial>(GetSelectedEntity())) {
//             if (ImGui::CollapsingHeader(ICON_LC_BRUSH " MATERIAL", ImGuiTreeNodeFlags_DefaultOpen)) {
//                 ImGui::Text("Material Properties");

//                 ImGui::ColorEdit3("Albedo Color", reinterpret_cast<float*>(&material->albedoColor));

//                 if (material->albedoTexture) {
//                     ImGui::Text("Albedo Texture: ");
//                     ImGui::Image(reinterpret_cast<ImTextureID>(material->albedoTexture->GetDescriptorSet()), ImVec2(100, 100));  // Display the texture
//                 } else {
//                     ImGui::Text("No Albedo Texture");
//                 }
//                 if (ImGui::Button("Change Albedo Texture")) {
//                     // Open the file dialog
//                     nfdchar_t* outPath = nullptr;
//                     if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
//                         std::string filepath = outPath;
//                         material->albedoTexture->MarkForDestruction();
//                         material->albedoTexturePath = filepath;
//                         material->dirty = true;
//                     }
//                     free(outPath);
//                 }

//                 // Display and edit the metallic factor
//                 ImGui::SliderFloat("Metallic", &material->metallic, 0.0f, 1.0f);

//                 // Display the metallic-roughness texture and allow changing it
//                 if (material->metallicRoughnessTexture) {
//                     ImGui::Text("Metallic Roughness Texture: ");
//                     ImGui::Image(reinterpret_cast<ImTextureID>(material->metallicRoughnessTexture->GetDescriptorSet()), ImVec2(100, 100));
//                 } else {
//                     ImGui::Text("No Metallic Roughness Texture");
//                 }
//                 if (ImGui::Button("Change Metallic Roughness Texture")) {
//                     // Open the file dialog
//                     nfdchar_t* outPath = nullptr;
//                     if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
//                         std::string filepath = outPath;
//                         material->metallicRoughnessTexture->MarkForDestruction();
//                         material->metallicRoughnessTexturePath = filepath;
//                         material->dirty = true;
//                     }
//                     free(outPath);  // Free the path memory
//                 }

//                 // Display and edit the roughness factor
//                 ImGui::SliderFloat("Roughness", &material->roughness, 0.0f, 1.0f);

//                 // Display the normal map and allow changing it
//                 if (material->normalTexture) {
//                     ImGui::Text("Normal Texture: ");
//                     ImGui::Image(reinterpret_cast<ImTextureID>(material->normalTexture->GetDescriptorSet()), ImVec2(100, 100));
//                 } else {
//                     ImGui::Text("No Normal Texture");
//                 }

//                 if (ImGui::Button("Change Normal Texture")) {
//                     // Open the file dialog
//                     nfdchar_t* outPath = nullptr;
//                     if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
//                         std::string filepath = outPath;
//                         material->normalTexture->MarkForDestruction();
//                         material->normalTexturePath = filepath;
//                         material->dirty = true;
//                     }
//                     free(outPath);  // Free the path memory
//                 }

//                 ImGui::SameLine();
//                 ImGui::Checkbox("Invert Normal", reinterpret_cast<bool*>(&material->invertNormalMap));

//                 // Display and edit the emission color
//                 ImGui::ColorEdit3("Emission Color", reinterpret_cast<float*>(&material->emissionColor));

//                 // Display the emissive texture and allow changing it
//                 if (material->emissiveTexture) {
//                     ImGui::Text("Emissive Texture: ");
//                     ImGui::Image(reinterpret_cast<ImTextureID>(material->emissiveTexture->GetDescriptorSet()), ImVec2(100, 100));
//                 } else {
//                     ImGui::Text("No Emissive Texture");
//                 }
//                 if (ImGui::Button("Change Emissive Texture")) {
//                     // Open the file dialog
//                     nfdchar_t* outPath = nullptr;
//                     if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
//                         std::string filepath = outPath;
//                         material->emissiveTexture->MarkForDestruction();
//                         material->emissiveTexturePath = filepath;
//                         material->dirty = true;
//                     }
//                     free(outPath);  // Free the path memory
//                 }

//                 // Display and edit the AO (ambient occlusion) factor
//                 ImGui::SliderFloat("AO Intensity", &material->aoIntensity, 0.0f, 1.0f);

//                 // Display the AO texture and allow changing it
//                 if (material->aoTexture) {
//                     ImGui::Text("AO Texture: ");
//                     ImGui::Image(reinterpret_cast<ImTextureID>(material->aoTexture->GetDescriptorSet()), ImVec2(100, 100));
//                 } else {
//                     ImGui::Text("No AO Texture");
//                 }
//                 if (ImGui::Button("Change AO Texture")) {
//                     // Open the file dialog
//                     nfdchar_t* outPath = nullptr;
//                     if (NFD_OpenDialog("png,jpg,tga", nullptr, &outPath) == NFD_OKAY) {
//                         std::string filepath = outPath;
//                         material->aoTexture->MarkForDestruction();
//                         material->aoTexturePath = filepath;
//                         material->dirty = true;
//                     }
//                     free(outPath);  // Free the path memory
//                 }

//                 // Display the emissive intensity slider
//                 ImGui::SliderFloat("Emissive Intensity", &material->emissiveIntensity, 0.0f, 10.0f);
//             }
//         }


//             // Light Component
//             if (auto* light = m_Registry.try_get<Engine::Light>(GetSelectedEntity())) {
//                 if (ImGui::CollapsingHeader(ICON_LC_SUN " LIGHT", ImGuiTreeNodeFlags_DefaultOpen)) {
//                     ImGui::Text("Light Type");
//                     ImGui::Combo("##LightType", reinterpret_cast<int*>(&light->type), "Directional\0Point\0Spot\0\0");

//                     ImGui::Text("Color");
//                     ImGui::ColorEdit3("##ColorPicker", reinterpret_cast<float*>(&light->color));

//                     ImGui::Text("Position");
//                     ImGui::DragFloat3("##Position", reinterpret_cast<float*>(&light->position), 0.1f);

//                     ImGui::Text("Intensity");
//                     ImGui::SliderFloat("##Intensity", &light->intensity, 0.0f, 10.0f);
//                 }
//             }
//         }
//     }
//     ImGui::End();

//     //Viewport
//     //TODO: Refactor this, not really working as intended right now
//     ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
//     if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar)) {

//         static bool isDragging = false;
//         static bool skipNextMouseDelta = false;

//         auto& camera = sceneContext.GetScene().GetActiveCamera();
//         if (camera.GetType() == Engine::Camera::CameraType::Free) {
//             if (ImGui::IsWindowHovered()) {
//                 if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
//                     m_Window.SetInputMode(GLFW_CURSOR_DISABLED);
//                     isDragging = true;
//                     skipNextMouseDelta = true;

//                     ImVec2 winPos = ImGui::GetWindowPos();
//                     ImVec2 winSize = ImGui::GetWindowSize();
//                     ImVec2 center = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f);

//                     m_Window.SetCursorPosition(center.x, center.y);
//                 }

//                 if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
//                     m_Window.SetInputMode(GLFW_CURSOR_NORMAL);
//                     isDragging = false;
//                 }

//                 if (isDragging) {
//                     ImVec2 windowPos = ImGui::GetWindowPos();      // screen coords
//                     ImVec2 windowSize = ImGui::GetWindowSize();
//                     ImVec2 center = { windowPos.x + windowSize.x * 0.5f, windowPos.y + windowSize.y * 0.5f };

//                     double xpos, ypos;
//                     m_Window.GetCursorPosition(xpos, ypos);

//                     if (!skipNextMouseDelta)
//                     {
//                         double deltaX = xpos - center.x;
//                         double deltaY = ypos - center.y;

//                         sceneContext.GetScene().GetActiveCamera().Rotate(
//                             deltaX * sceneContext.GetScene().GetActiveCamera().GetRotationSpeed(),
//                             deltaY * sceneContext.GetScene().GetActiveCamera().GetRotationSpeed());
//                     }
//                     else
//                     {
//                         skipNextMouseDelta = false; // skip this one frame only
//                     }

//                     m_Window.SetCursorPosition(center.x, center.y);

//                     if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_W))) {
//                         sceneContext.GetScene().GetActiveCamera().MoveForward(deltaTime);
//                     }
//                     if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_S))) {
//                         sceneContext.GetScene().GetActiveCamera().MoveBackward(deltaTime);
//                     }
//                     if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_D))) {
//                         sceneContext.GetScene().GetActiveCamera().MoveRight(deltaTime);
//                     }
//                     if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_A))) {
//                         sceneContext.GetScene().GetActiveCamera().MoveLeft(deltaTime);
//                     }
//                     if (ImGui::IsKeyDown(ImGui::GetKeyIndex(ImGuiKey_LeftShift))) {
//                         sceneContext.GetScene().GetActiveCamera().SpeedUp();
//                     }
//                     else {
//                         sceneContext.GetScene().GetActiveCamera().ResetSpeed();
//                     }
//                 }
//             }
//         }
//         else if (camera.GetType() == Engine::Camera::CameraType::Orbit) {
//             if (camera.OrbitAroundEntity()) {
//                 Engine::Entity ent = {m_Registry, GetSelectedEntity()};
//                 if (ent.HasComponent<Engine::Transform>()) {
//                     camera.SetOrbitTarget(ent.GetComponent<Engine::Transform>().position);
//                 }
//             }


//             if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
//                 m_Window.SetInputMode(GLFW_CURSOR_DISABLED);
//                 isDragging = true;
//                 skipNextMouseDelta = true;

//                 ImVec2 winPos = ImGui::GetWindowPos();
//                 ImVec2 winSize = ImGui::GetWindowSize();
//                 ImVec2 center = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f);

//                 m_Window.SetCursorPosition(center.x, center.y);
//             }

//             if (ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
//                 m_Window.SetInputMode(GLFW_CURSOR_NORMAL);
//                 isDragging = false;
//             }

//             if (isDragging) {
//                 ImVec2 winPos = ImGui::GetWindowPos();
//                 ImVec2 winSize = ImGui::GetWindowSize();
//                 ImVec2 center = ImVec2(winPos.x + winSize.x * 0.5f, winPos.y + winSize.y * 0.5f);

//                 double xpos, ypos;
//                 m_Window.GetCursorPosition(xpos, ypos);

//                 if (!skipNextMouseDelta) {
//                     double deltaX = xpos - center.x;
//                     double deltaY = ypos - center.y;

//                     camera.OrbitRotate(
//                         static_cast<float>(deltaX) * camera.GetRotationSpeed(),
//                         static_cast<float>(-deltaY) * camera.GetRotationSpeed());
//                 } else {
//                     skipNextMouseDelta = false;
//                 }

//                 m_Window.SetCursorPosition(center.x, center.y);
//             }

//             float scroll = ImGui::GetIO().MouseWheel;
//             if (camera.GetType() == Engine::Camera::CameraType::Orbit && scroll != 0.0f) {
//                 camera.OrbitZoom(-scroll * 2.0f);
//             }
//         }

//         static ImGuizmo::OPERATION m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE; // Default operation

//         ImGui::SetNextWindowPos({ImGui::GetWindowPos().x + 10, ImGui::GetWindowPos().y + 30}, ImGuiCond_Always);
//         ImGui::SetNextWindowSize(ImVec2(150, 50)); // Set the window size
//         ImGui::Begin("Gizmo Controls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

//         // Calculate the center position for the buttons
//         ImVec2 windowSize = ImGui::GetWindowSize();
//         ImVec2 buttonSize(30, 30); // Assuming each button is 30x30 pixels
//         float spacing = 10; // Space between buttons

//         // Calculate the total width of the buttons including spacing
//         float totalButtonsWidth = (buttonSize.x * 3) + (spacing * 2);

//         // Calculate the starting X position for the first button
//         float startX = (windowSize.x - totalButtonsWidth) / 2;

//         // Calculate the starting Y position for the buttons
//         float startY = (windowSize.y - buttonSize.y) / 2;

//         // Set the cursor position to the center
//         ImGui::SetCursorPos(ImVec2(startX, startY));

//         if (ImGui::Button(ICON_LC_MOVE_3D)) {
//             m_CurrentOperation = ImGuizmo::OPERATION::TRANSLATE;
//         }
//         ImGui::SameLine();
//         ImGui::SetCursorPosX(startX + buttonSize.x + spacing);

//         if (ImGui::Button(ICON_LC_SCALE_3D)) {
//             m_CurrentOperation = ImGuizmo::OPERATION::SCALE;
//         }
//         ImGui::SameLine();
//         ImGui::SetCursorPosX(startX + (buttonSize.x + spacing) * 2);

//         if (ImGui::Button(ICON_LC_ROTATE_3D)) {
//             m_CurrentOperation = ImGuizmo::OPERATION::ROTATE;
//         }

//         ImGui::End();



//         // Render scene to viewport
//         ImVec2 viewportSize = ImGui::GetContentRegionAvail();
//         float newWidth = viewportSize.x;
//         float newHeight = viewportSize.y;

//         if (newWidth != m_LastViewportSize.x || newHeight != m_LastViewportSize.y) {
//             m_ViewportResized = true;
//             m_ViewportExtent = { static_cast<uint32_t>(viewportSize.x), static_cast<uint32_t>(viewportSize.y) };
//             m_LastViewportSize = viewportSize;
//         }

//         auto textureId = reinterpret_cast<ImTextureID>(m_FinalImage);
//         ImGui::Image(textureId, { viewportSize.x, viewportSize.y }, { 0, 1 }, { 1, 0 });

//         // Gizmos
//         if (GetSelectedEntity() != entt::null && m_Registry.valid(GetSelectedEntity())) {
//             Engine::Entity ent = {m_Registry, GetSelectedEntity()};

//             if (auto* transform = m_Registry.try_get<Engine::Transform>(GetSelectedEntity())) {
//                 ImGuizmo::SetOrthographic(false);
//                 ImGuizmo::SetDrawlist();
//                 ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());
//                 auto cameraView = sceneContext.GetScene().GetActiveCamera().GetView();
//                 auto cameraProjection = sceneContext.GetScene().GetActiveCamera().GetProjection();
//                 auto transformComponent = transform->TransformMatrix();

//                 ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection),
//                                      m_CurrentOperation, ImGuizmo::MODE::LOCAL, glm::value_ptr(transformComponent));

//                 if (ImGuizmo::IsUsing()) {

//                     glm::vec3 translation, rotation, scale;
//                     ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transformComponent),
//                             glm::value_ptr(translation),
//                             glm::value_ptr(rotation),
//                             glm::value_ptr(scale));

//                     transform->position = translation;
//                     transform->scale = scale;
//                     transform->rotation = glm::quat(glm::radians(rotation));
//                 }
//             }
//         }
//     }
//     ImGui::End();

//     ImGui::PopStyleVar();

//     ImGui::End();
//     ImGui::Render();
//     ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
//     ImGui::EndFrame();
// }
