#include "UI/Panels/Properties.h"
#include "Aquila/Core/Application.h"
#include "Aquila/Core/Defines.h"
#include "Aquila/Events/SceneEvent.h"
#include "Aquila/Events/AssetEvent.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "Aquila/Rendering/Systems/CascadedShadowMapsRenderingSystem.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MaterialComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Utilities/Profiler.h"
#include "UI/ImGuiUtils.h"
#include "UI/Managers/FontManager.h"
#include "imgui.h"
#include "lucide.h"
#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"

namespace Editor {

PropertiesPanel::PropertiesPanel(Aquila::Core::Application &app) : Layer("Properties"), m_App(app) {
	AQUILA_LOG_INFO("PropertiesPanel created");
}

void PropertiesPanel::OnAttach() {
	AQUILA_LOG_INFO("PropertiesPanel attached");
	m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
	m_SelectedAssetPath.clear();
	m_SelectedAssetExtension.clear();
}

void PropertiesPanel::OnDetach() {
	AQUILA_LOG_INFO("PropertiesPanel detached");
}

void PropertiesPanel::OnUpdate(f32 deltaTime) {}

// ─────────────────────────────────────────────────────────────────────────────
// HELPERS: extract filename / stem from a virtual path
// ─────────────────────────────────────────────────────────────────────────────
static std::string ExtractFilename(const std::string &path) {
	size_t slash = path.find_last_of("/\\");
	return (slash != std::string::npos) ? path.substr(slash + 1) : path;
}

static std::string ExtractStem(const std::string &path) {
	std::string name = ExtractFilename(path);
	size_t dot = name.find_last_of('.');
	return (dot != std::string::npos) ? name.substr(0, dot) : name;
}

// ─────────────────────────────────────────────────────────────────────────────
// MAIN RENDER LOOP
// ─────────────────────────────────────────────────────────────────────────────
void PropertiesPanel::OnImGuiRender() {
	if (auto *scene = m_App.GetAssetManager().GetActiveScene(); !scene) {
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
	ImGui::Begin(ICON_LC_SETTINGS " Details", nullptr,
				 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();

	// ── Priority: Entity > Asset > Empty ──
	bool hasEntity = !m_SelectedEntity.IsNull() && m_SelectedEntity.IsValid();
	bool hasAsset = !m_SelectedAssetPath.empty();

	if (!hasEntity && !hasAsset) {
		const char *msg = "Select an actor or asset to view details";

		ImGui::PushFont(UI::FontManager::Get().GetFont14());
		const ImVec2 winSize = ImGui::GetContentRegionAvail();
		const ImVec2 textSize = ImGui::CalcTextSize(msg);
		ImGui::SetCursorPos({ (winSize.x - textSize.x) * 0.5f, (winSize.y - textSize.y) * 0.5f });
		ImGui::TextDisabled("%s", msg);
		ImGui::PopFont();
		ImGui::End();
		return;
	}

	if (!hasEntity && hasAsset) {
		ImGui::BeginChild("PropertiesScroll", ImVec2(0, 0), ImGuiChildFlags_None);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

		if (m_SelectedAssetExtension == ".png" || m_SelectedAssetExtension == ".jpg" ||
			m_SelectedAssetExtension == ".tga" || m_SelectedAssetExtension == ".hdr") {
			DrawAsset_Texture();
		} else if (m_SelectedAssetExtension == ".gltf" || m_SelectedAssetExtension == ".glb" ||
				   m_SelectedAssetExtension == ".obj" || m_SelectedAssetExtension == ".fbx") {
			DrawAsset_Mesh();
		} else if (m_SelectedAssetExtension == ".aqmat") {
			DrawAsset_Material();
		} else if (m_SelectedAssetExtension == ".slang") {
			DrawAsset_Shader();
		} else {
			// Generic file – just show path
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), ICON_LC_FILE "  %s",
							   ExtractFilename(m_SelectedAssetPath).c_str());
			ImGui::Spacing();
			ImGui::TextDisabled("Path: %s", m_SelectedAssetPath.c_str());
		}

		ImGui::PopStyleVar(2);
		ImGui::EndChild();
		ImGui::End();
		return;
	}

	// ── Otherwise render the normal entity inspector ──
	ImGui::BeginChild("PropertiesScroll", ImVec2(0, 0), ImGuiChildFlags_None);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

	if (m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::MetadataComponent>()) {
		PROFILE_SCOPE("DrawMetaData");
		DrawComponent_Metadata(m_SelectedEntity);
	}
	if (m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
		PROFILE_SCOPE("DrawTransform");
		DrawComponent_Transform(m_SelectedEntity);
	}
	if (m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::MeshComponent>()) {
		PROFILE_SCOPE("DrawMeshComponent");
		DrawComponent_Mesh(m_SelectedEntity);
	}
	if (m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::CameraComponent>()) {
		PROFILE_SCOPE("DrawCameraComponent");
		DrawComponent_Camera(m_SelectedEntity);
	}
	if (m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::LightComponent>()) {
		PROFILE_SCOPE("DrawLightComponent");
		DrawComponent_Light(m_SelectedEntity);
	}
	if (m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::MaterialComponent>()) {
		PROFILE_SCOPE("DrawMaterialComponent");
		DrawComponent_Material(m_SelectedEntity);
	}
	if (m_SelectedEntity.HasComponent<Aquila::SceneManagement::Components::SkyLightComponent>()) {
		PROFILE_SCOPE("DrawSkyLightComponent");
		DrawComponent_SkyLight(m_SelectedEntity);
	}

	ImGui::Spacing();
	ImGui::Spacing();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.48f, 0.0f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.58f, 0.0f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.68f, 0.0f, 1.0f));

	ImGui::SetCursorPosX(8);
	if (ImGui::Button(ICON_LC_PLUS " Add Component", ImVec2(ImGui::GetContentRegionAvail().x - 16, 0))) {
		ImGui::OpenPopup("AddComponentPopup");
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();

	if (ImGui::BeginPopup("AddComponentPopup")) {
		DrawAddComponentMenu(m_SelectedEntity);
		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(2);
	ImGui::EndChild();
	ImGui::End();
}

// ═════════════════════════════════════════════════════════════════════════════
// ASSET INSPECTORS
// ═════════════════════════════════════════════════════════════════════════════

void PropertiesPanel::DrawAsset_Texture() {
	auto &assetManager = m_App.GetAssetManager();
	std::string filename = ExtractFilename(m_SelectedAssetPath);
	std::string stem = ExtractStem(m_SelectedAssetPath);
	bool isHDR = (m_SelectedAssetExtension == ".hdr");

	// ─── Header ───
	const char *icon = isHDR ? ICON_LC_SUN : ICON_LC_IMAGE;
	DrawComponentHeader(icon, isHDR ? "HDR TEXTURE" : "TEXTURE", "TextureAssetMenu");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));
	ImGui::Indent(8);
	ImGui::Spacing();

	// ─── Info table ───
	if (ImGui::BeginTable("TextureInfo", 2, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		// Name
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Name");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", stem.c_str());

		// Path
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Path");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", m_SelectedAssetPath.c_str());

		// Type
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Type");
		ImGui::TableNextColumn();
		ImGui::Text("%s", isHDR ? "HDR Environment" : "2D Texture");

		// Format
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Format");
		ImGui::TableNextColumn();
		ImGui::Text("%s", isHDR ? "R32G32B32A32 f32" : "R8G8B8A8");

		// Dimensions (if already loaded)
		auto texture = assetManager.TryGetTexture(m_SelectedAssetPath);
		if (texture) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Dimensions");
			ImGui::TableNextColumn();
			ImGui::Text("%u x %u", texture->GetWidth(), texture->GetHeight());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Mip Levels");
			ImGui::TableNextColumn();
			ImGui::Text("%u", texture->GetMipLevels());
		}

		ImGui::EndTable();
	}

	auto texture = assetManager.TryGetTexture(m_SelectedAssetPath);
	if (texture) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Preview");
		ImGui::Spacing();

		ImTextureID tid = reinterpret_cast<ImTextureID>(texture->GetDescriptorSet());
		f32 avail = ImGui::GetContentRegionAvail().x - 16.0f;
		// Keep aspect ratio, max 256 tall
		f32 w = texture->GetWidth();
		f32 h = texture->GetHeight();
		f32 scale = std::min(avail / w, 256.0f / h);
		ImGui::Image(tid, ImVec2(w * scale, h * scale));
	} else {
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), ICON_LC_TRIANGLE_ALERT " Not yet loaded into GPU");
	}

	ImGui::Spacing();
	ImGui::Unindent(8);
	ImGui::PopStyleVar(2);
}

void PropertiesPanel::DrawAsset_Mesh() {
	std::string filename = ExtractFilename(m_SelectedAssetPath);
	std::string stem = ExtractStem(m_SelectedAssetPath);

	DrawComponentHeader(ICON_LC_BOX, "MESH", "MeshAssetMenu");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));
	ImGui::Indent(8);
	ImGui::Spacing();

	if (ImGui::BeginTable("MeshInfo", 2, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Name");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", stem.c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Path");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", m_SelectedAssetPath.c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Format");
		ImGui::TableNextColumn();
		if (m_SelectedAssetExtension == ".gltf")
			ImGui::Text("glTF (JSON)");
		else if (m_SelectedAssetExtension == ".glb")
			ImGui::Text("glTF (Binary)");
		else if (m_SelectedAssetExtension == ".obj")
			ImGui::Text("Wavefront OBJ");
		else if (m_SelectedAssetExtension == ".fbx")
			ImGui::Text("Autodesk FBX");
		else
			ImGui::Text("Unknown");

		auto &assetManager = m_App.GetAssetManager();
		auto mesh = assetManager.TryGetMesh(m_SelectedAssetPath);
		if (mesh) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Vertices");
			ImGui::TableNextColumn();
			ImGui::Text("%u", mesh->GetVertexCount());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Indices");
			ImGui::TableNextColumn();
			ImGui::Text("%u", mesh->GetIndexCount());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Primitives");
			ImGui::TableNextColumn();
			ImGui::Text("%u", (uint32)mesh->GetPrimitives().size());
		} else {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Status");
			ImGui::TableNextColumn();
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), ICON_LC_TRIANGLE_ALERT " Not loaded");
		}

		ImGui::EndTable();
	}

	ImGui::Spacing();
	ImGui::Unindent(8);
	ImGui::PopStyleVar(2);
}

// ─────────────────────────────────────────────────────────────────────────────
// MATERIAL ASSET INSPECTOR  (Godot-style: create material, then bind shader)
// ─────────────────────────────────────────────────────────────────────────────
void PropertiesPanel::DrawAsset_Material() {
	auto &assetManager = m_App.GetAssetManager();
	auto &matSystem = m_App.GetMaterialSystem();

	std::string stem = ExtractStem(m_SelectedAssetPath);

	auto material = assetManager.LoadMaterial(m_SelectedAssetPath);
	if (!material) {
		DrawComponentHeader(ICON_LC_PALETTE, "MATERIAL", "MatAssetMenu");
		ImGui::Spacing();
		ImGui::Indent(8);
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), ICON_LC_TRIANGLE_ALERT " Failed to load material");
		ImGui::Unindent(8);
		return;
	}

	std::vector<ComponentMenuAction> actions = { ComponentMenuAction(
		ComponentMenuAction::CUSTOM, ICON_LC_SAVE, "Save", [&]() {
			if (assetManager.SaveMaterialAsset(material, m_SelectedAssetPath)) {
				AQUILA_LOG_INFO("Material saved: {}", m_SelectedAssetPath);
			} else {
				AQUILA_LOG_ERROR("Failed to save material: {}", m_SelectedAssetPath);
			}
		}) };

	const bool headerOpen = DrawComponentHeader(ICON_LC_PALETTE, "MATERIAL", "MatAssetMenu", actions);
	if (!headerOpen) {
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));
	ImGui::Indent(8);
	ImGui::Spacing();

	// ─── Basic info ───
	if (ImGui::BeginTable("MatBasicInfo", 2, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Name");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", material->name.c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Path");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", m_SelectedAssetPath.c_str());

		ImGui::EndTable();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// ═══════════════════════════════════════════════════════════
	// SHADER SLOT  ─  the core Godot-style binding widget
	// ═══════════════════════════════════════════════════════════
	ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Shader");
	ImGui::Spacing();

	std::string currentShaderPath;
	std::string currentShaderName = "None";
	bool hasShader = false;

	if (material->GetTemplate() && material->GetTemplate()->shader) {
		hasShader = true;
		currentShaderName = material->GetTemplate()->shader->m_Name;
		currentShaderPath = material->GetShaderAssetPath();
		if (currentShaderPath.empty())
			currentShaderPath = currentShaderName;
	}

	if (ImGui::BeginTable("ShaderSlot", 2, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Shader");

		ImGui::TableNextColumn();

		ImGuiUtils::DragDrop::AssetDropZone(
			"ShaderAssetSlot", "Shader", currentShaderPath, "SHADER_ASSET",
			[&](const char *path) {
				// User dropped a shader onto this material – bind it
				auto shader = assetManager.LoadShader(path);
				if (shader) {
					material->SetShader(shader);
					material->SetShaderAsset(path);

					// Allocate descriptors for the new shader layout
					matSystem.AllocateDescriptorsForMaterial(material, assetManager.GetActiveScene());

					// Persist immediately
					if (assetManager.SaveMaterialAsset(material, m_SelectedAssetPath)) {
						AQUILA_LOG_INFO("Shader '{}' bound to material '{}' and saved", path, material->name);
					}
				} else {
					AQUILA_LOG_ERROR("Failed to load shader: {}", path);
				}
			},
			hasShader ? [&]() {
				// Clear shader
				if (material->GetTemplate()) {
					material->GetTemplate()->shader = nullptr;
					material->SetShaderAsset("");
					material->MarkPipelineDirty();
					assetManager.SaveMaterialAsset(material, m_SelectedAssetPath);
					AQUILA_LOG_INFO("Shader unbound from material '{}'", material->name);
				}
			} : std::function<void()>(nullptr)
		);

		if (hasShader && material->GetTemplate()->shader) {
			auto &shader = material->GetTemplate()->shader;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Stages");
			ImGui::TableNextColumn();
			std::string stageStr;
			for (auto &stage : shader->m_Stages) {
				if (!stageStr.empty())
					stageStr += " | ";
				if (stage.stage == VK_SHADER_STAGE_VERTEX_BIT)
					stageStr += "Vertex";
				if (stage.stage == VK_SHADER_STAGE_FRAGMENT_BIT)
					stageStr += "Fragment";
				if (stage.stage == VK_SHADER_STAGE_COMPUTE_BIT)
					stageStr += "Compute";
			}
			ImGui::TextDisabled("%s", stageStr.c_str());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Pipeline");
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", material->GetPipeline() != VK_NULL_HANDLE ? "Valid" : "Not built");
		}

		ImGui::EndTable();
	}

	if (!hasShader) {
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), ICON_LC_TRIANGLE_ALERT " No shader assigned.");
		ImGui::TextDisabled("Drag a .vert or .frag file onto the Shader slot above.");
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (hasShader) {
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Properties");
		ImGui::Spacing();

		auto properties = material->GetAllProperties();

		if (properties.empty()) {
			ImGui::TextDisabled("No editable properties.");
		} else if (ImGui::BeginTable("MatProps", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			for (auto &[name, prop] : properties) {
				if (!prop.m_IsEditable || prop.m_Name.empty()) {
					continue;
				}

				ImGui::PushID(name.c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", prop.m_DisplayName.c_str());

				ImGui::TableNextColumn();

				switch (prop.m_Type) {
				case Aquila::Graphics::Material::ParameterType::Float: {
					f32 val = std::get<f32>(prop.m_Value);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::SliderFloat("##val", &val, prop.m_MinValue, prop.m_MaxValue, "%.3f")) {
						material->SetParameter(name, val);
					}
					break;
				}
				case Aquila::Graphics::Material::ParameterType::Color: {
					vec4 color = std::get<vec4>(prop.m_Value);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::ColorEdit4("##val", glm::value_ptr(color),
										  ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar)) {
						material->SetParameter(name, color);
					}
					break;
				}
				case Aquila::Graphics::Material::ParameterType::Texture2D: {
					auto texture = material->GetTexture(name);
					const std::string texLabel =
						texture && !material->IsFallbackTexture(name, texture) ? "Texture" : "";

					ImGuiUtils::DragDrop::AssetDropZone(
						prop.m_DisplayName.c_str(), "Texture2D", texLabel, "TEXTURE_ASSET",
						[&](const char *path) {
							VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
							if (name.find("albedo") != std::string::npos || name.find("Albedo") != std::string::npos ||
								name.find("diffuse") != std::string::npos ||
								name.find("baseColor") != std::string::npos) {
								format = VK_FORMAT_R8G8B8A8_SRGB;
							}
							if (auto newTex = assetManager.LoadTexture(path, format)) {
								material->SetParameter(name, newTex);
								AQUILA_LOG_INFO("Texture loaded: {}", path);
							}
						},
						[&]() { material->SetParameter(name, Ref<Aquila::Graphics::Resources::Texture2D>(nullptr)); });
					break;
				}
				case Aquila::Graphics::Material::ParameterType::Int:
				case Aquila::Graphics::Material::ParameterType::Bool:
				case Aquila::Graphics::Material::ParameterType::Vec2:
				case Aquila::Graphics::Material::ParameterType::Vec3:
				case Aquila::Graphics::Material::ParameterType::Vec4:
					break;
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ─── Render state ───
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Render State");
		ImGui::Spacing();

		auto renderState = material->GetRenderState();
		bool stateChanged = false;

		if (ImGui::BeginTable("MatRenderState", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Blend Mode");
			ImGui::TableNextColumn();
			{
				const char *blendModes[] = { "Opaque", "Alpha Blend", "Additive", "Multiply" };
				int cur = static_cast<int>(renderState.m_BlendMode);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Combo("##Blend", &cur, blendModes, IM_ARRAYSIZE(blendModes))) {
					renderState.m_BlendMode = static_cast<Aquila::Graphics::Material::BlendMode>(cur);
					stateChanged = true;
				}
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Cull Mode");
			ImGui::TableNextColumn();
			{
				const char *cullModes[] = { "None", "Front", "Back" };
				int cur = static_cast<int>(renderState.m_CullMode);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::Combo("##Cull", &cur, cullModes, IM_ARRAYSIZE(cullModes))) {
					renderState.m_CullMode = static_cast<Aquila::Graphics::Material::CullMode>(cur);
					stateChanged = true;
				}
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Depth Test");
			ImGui::TableNextColumn();
			if (ImGui::Checkbox("##DepthTest", &renderState.m_DepthTest))
				stateChanged = true;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Depth Write");
			ImGui::TableNextColumn();
			if (ImGui::Checkbox("##DepthWrite", &renderState.m_DepthWrite))
				stateChanged = true;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Wireframe");
			ImGui::TableNextColumn();
			if (ImGui::Checkbox("##Wireframe", &renderState.m_Wireframe))
				stateChanged = true;

			if (renderState.m_Wireframe) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Line Width");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::SliderFloat("##LineWidth", &renderState.m_LineWidth, 1.0f, 10.0f))
					stateChanged = true;
			}

			ImGui::EndTable();
		}

		if (stateChanged) {
			material->SetRenderState(renderState);
		}
	}

	ImGui::Spacing();
	ImGui::Unindent(8);
	ImGui::PopStyleVar(2);
}

void PropertiesPanel::DrawAsset_Shader() {
	auto &assetManager = m_App.GetAssetManager();

	// .slang files are self-contained
	std::string slangPath = m_SelectedAssetPath;

	auto shader = assetManager.TryGetShader(slangPath);
	if (!shader)
		shader = assetManager.LoadShader(slangPath);

	DrawComponentHeader(ICON_LC_CODE, "SHADER", "ShaderAssetMenu");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));
	ImGui::Indent(8);
	ImGui::Spacing();

	if (ImGui::BeginTable("ShaderInfo", 2, ImGuiTableFlags_SizingStretchProp)) {
		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Name");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", shader ? shader->m_Name.c_str() : ExtractStem(m_SelectedAssetPath).c_str());

		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Source");
		ImGui::TableNextColumn();
		ImGui::TextDisabled("%s", ExtractFilename(slangPath).c_str());

		if (shader) {
			// Stages
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Stages");
			ImGui::TableNextColumn();
			{
				std::string stageStr;
				for (const auto &stage : shader->m_Stages) {
					if (!stageStr.empty())
						stageStr += " | ";
					if (stage.stage == VK_SHADER_STAGE_VERTEX_BIT)
						stageStr += "Vertex";
					if (stage.stage == VK_SHADER_STAGE_FRAGMENT_BIT)
						stageStr += "Fragment";
					if (stage.stage == VK_SHADER_STAGE_COMPUTE_BIT)
						stageStr += "Compute";
				}
				ImGui::Text("%s", stageStr.c_str());
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Mat Layout");
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", shader->GetDescriptorSetLayout() != VK_NULL_HANDLE ? "Valid" : "None");

		} else {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Status");
			ImGui::TableNextColumn();
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), ICON_LC_TRIANGLE_ALERT " Failed to compile");
		}

		ImGui::EndTable();
	}

	// ─── Reflected bindings — read from ShaderProgram, no manual SPIRV-Reflect ───
	if (shader && !shader->GetReflectedBindings().empty()) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Reflected Bindings (Material Set)");
		ImGui::Spacing();

		if (ImGui::BeginTable("ReflectedBindings", 4,
							  ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
			ImGui::TableSetupColumn("Set", ImGuiTableColumnFlags_WidthFixed, 40.0f);
			ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthFixed, 60.0f);
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableHeadersRow();

			for (const auto &rb : shader->GetReflectedBindings()) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%u", rb.set);
				ImGui::TableNextColumn();
				ImGui::Text("%u", rb.bindingIndex);
				ImGui::TableNextColumn();
				ImGui::Text("%s", rb.name.empty() ? "(unnamed)" : rb.name.c_str());
				ImGui::TableNextColumn();

				using RBT = Aquila::Graphics::Shader::ShaderProgram::ReflectedBindingType;
				switch (rb.type) {
				case RBT::CombinedImageSampler:
					ImGui::TextDisabled("Sampler2D");
					break;
				case RBT::UniformBuffer:
					ImGui::TextDisabled("UBO");
					break;
				case RBT::StorageBuffer:
					ImGui::TextDisabled("SSBO");
					break;
				default:
					ImGui::TextDisabled("Other");
					break;
				}
			}

			ImGui::EndTable();
		}
	}

	// ─── Quick-create material button ───
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (shader) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.48f, 0.0f, 0.8f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.58f, 0.0f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.68f, 0.0f, 1.0f));

		if (ImGui::Button(ICON_LC_PLUS " Create Material from this Shader", ImVec2(-FLT_MIN, 0))) {
			m_ShowCreateMaterialFromShaderPopup = true;
			std::string defaultName = shader->m_Name + "_mat";
			strncpy(m_CreateMatNameBuffer, defaultName.c_str(), sizeof(m_CreateMatNameBuffer) - 1);
			m_CreateMatNameBuffer[sizeof(m_CreateMatNameBuffer) - 1] = '\0';
			m_CreateMatShaderBasePath = slangPath;
		}

		ImGui::PopStyleColor(3);

		if (m_ShowCreateMaterialFromShaderPopup) {
			ImGui::OpenPopup("CreateMatFromShader");
			m_ShowCreateMaterialFromShaderPopup = false;
		}

		if (ImGui::BeginPopupModal("CreateMatFromShader", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Create a new material using this shader");
			ImGui::Separator();
			ImGui::Text("Material name:");
			ImGui::SetNextItemWidth(280.0f);
			ImGui::InputText("##matname", m_CreateMatNameBuffer, sizeof(m_CreateMatNameBuffer));
			ImGui::Spacing();

			bool canCreate = strlen(m_CreateMatNameBuffer) > 0;
			if (!canCreate)
				ImGui::BeginDisabled();

			if (ImGui::Button("Create", ImVec2(120, 0))) {
				auto &matLib = m_App.GetMaterialSystem().GetLibrary();
				std::string matName = m_CreateMatNameBuffer;

				std::string saveDir = m_CreateMatShaderBasePath;
				size_t lastSlash = saveDir.find_last_of('/');
				saveDir = (lastSlash != std::string::npos) ? saveDir.substr(0, lastSlash + 1) : "assets://materials/";
				std::string savePath = saveDir + matName + ".aqmat";

				auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();
				for (int counter = 1; vfs->Exists(savePath) && counter < 1000; ++counter)
					savePath = saveDir + matName + "_" + std::to_string(counter) + ".aqmat";

				if (shader) {
					auto mat = matLib.CreateMaterialFromShader(matName, shader);
					if (mat) {
						mat->SetShaderAsset(m_CreateMatShaderBasePath);
						if (assetManager.SaveMaterialAsset(mat, savePath))
							AQUILA_LOG_INFO("Created material '{}' at '{}'", matName, savePath);
						else
							AQUILA_LOG_ERROR("Failed to save material to: {}", savePath);
					}
				}
				ImGui::CloseCurrentPopup();
			}

			if (!canCreate)
				ImGui::EndDisabled();

			ImGui::SameLine();
			if (ImGui::Button("Cancel", ImVec2(120, 0)))
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}
	}

	ImGui::Spacing();
	ImGui::Unindent(8);
	ImGui::PopStyleVar(2);
}

// ═════════════════════════════════════════════════════════════════════════════
// ENTITY COMPONENT INSPECTORS (unchanged logic, kept intact)
// ═════════════════════════════════════════════════════════════════════════════

void PropertiesPanel::DrawAddComponentMenu(Aquila::SceneManagement::Entity entity) {
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));

	if (!entity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
		if (ImGui::MenuItem(ICON_LC_MOVE_3D " Transform Component")) {
			entity.AddComponent<Aquila::SceneManagement::Components::TransformComponent>();
			ImGui::CloseCurrentPopup();
		}
	}

	if (!entity.HasComponent<Aquila::SceneManagement::Components::MeshComponent>()) {
		if (ImGui::MenuItem(ICON_LC_GRID_3X3 " Static Mesh Component")) {
			entity.AddComponent<Aquila::SceneManagement::Components::MeshComponent>();
			if (!entity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
				entity.AddComponent<Aquila::SceneManagement::Components::TransformComponent>();
			}
			ImGui::CloseCurrentPopup();
		}
	}

	if (!entity.HasComponent<Aquila::SceneManagement::Components::CameraComponent>()) {
		if (ImGui::MenuItem(ICON_LC_CAMERA " Camera Component")) {
			entity.AddComponent<Aquila::SceneManagement::Components::CameraComponent>();
			ImGui::CloseCurrentPopup();
		}
	}

	if (!entity.HasComponent<Aquila::SceneManagement::Components::MaterialComponent>()) {
		if (ImGui::MenuItem(ICON_LC_BRUSH " Material Component")) {
			entity.AddComponent<Aquila::SceneManagement::Components::MaterialComponent>();
			ImGui::CloseCurrentPopup();
		}
	}

	if (!entity.HasComponent<Aquila::SceneManagement::Components::LightComponent>()) {
		if (ImGui::MenuItem(ICON_LC_LIGHTBULB " Light Component")) {
			entity.AddComponent<Aquila::SceneManagement::Components::LightComponent>();
			if (!entity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
				entity.AddComponent<Aquila::SceneManagement::Components::TransformComponent>();
			}
			ImGui::CloseCurrentPopup();
		}
	}

	if (!entity.HasComponent<Aquila::SceneManagement::Components::SkyLightComponent>()) {
		if (ImGui::MenuItem(ICON_LC_SUN " Sky Lights Component")) {
			entity.AddComponent<Aquila::SceneManagement::Components::SkyLightComponent>();
			if (!entity.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
				entity.AddComponent<Aquila::SceneManagement::Components::TransformComponent>();
			}
			ImGui::CloseCurrentPopup();
		}
	}

	ImGui::PopStyleVar();
}

bool PropertiesPanel::DrawComponentHeader(const char *icon, const char *label, const char *menuId,
										  const std::vector<ComponentMenuAction> &menuActions) {
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.08f, 0.12f, 0.18f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.12f, 0.16f, 0.22f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.15f, 0.20f, 0.28f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

	const std::string headerLabel = std::string(icon) + "  " + std::string(label);
	const bool headerOpen =
		ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowOverlap |
														 ImGuiTreeNodeFlags_SpanAvailWidth);

	ImGui::PopStyleVar();
	ImGui::PopStyleColor(3);

	if (!menuActions.empty()) {
		constexpr f32 buttonSize = 20.0f;

		ImGui::SameLine(ImGui::GetContentRegionMax().x - buttonSize - 8.0f);

		ImVec2 buttonPos = ImGui::GetItemRectMax();
		buttonPos.x -= buttonSize + 8;
		buttonPos.y = ImGui::GetItemRectMin().y + (ImGui::GetItemRectSize().y - buttonSize) * 0.5f;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));

		const std::string buttonId = std::string(ICON_LC_ELLIPSIS_VERTICAL) + "##" + std::string(menuId);
		if (ImGui::Button(buttonId.c_str(), ImVec2(buttonSize, buttonSize))) {
			ImGui::OpenPopup(menuId);
		}

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();

		if (ImGui::BeginPopup(menuId)) {
			for (const auto &action : menuActions) {
				const bool isDestructive = (action.type == ComponentMenuAction::REMOVE_COMPONENT);

				if (isDestructive) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
				}

				const std::string menuLabel = std::string(action.icon) + " " + std::string(action.label);
				if (ImGui::MenuItem(menuLabel.c_str())) {
					if (action.callback) {
						action.callback();
					}
				}

				if (isDestructive) {
					ImGui::PopStyleColor();
				}
			}
			ImGui::EndPopup();
		}
	}

	return headerOpen;
}

void PropertiesPanel::UpdateChildrenTransforms(Aquila::SceneManagement::Entity parentEntity) {
	if (!parentEntity.IsValid() ||
		!parentEntity.HasAllComponents<Aquila::SceneManagement::Components::SceneNodeComponent,
									   Aquila::SceneManagement::Components::TransformComponent>()) {
		return;
	}

	auto &parentNode = parentEntity.GetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>();
	auto &parentTransform = parentEntity.GetComponent<Aquila::SceneManagement::Components::TransformComponent>();
	const glm::mat4 &parentWorldMatrix = parentTransform.GetWorldMatrix();

	for (auto child : parentNode.Children) {
		if (!child.IsValid() || !child.HasComponent<Aquila::SceneManagement::Components::TransformComponent>()) {
			continue;
		}

		auto &childTransform = child.GetComponent<Aquila::SceneManagement::Components::TransformComponent>();
		childTransform.UpdateWorldMatrix(parentWorldMatrix);

		UpdateChildrenTransforms(child);
	}
}

void PropertiesPanel::DrawComponent_SkyLight(Aquila::SceneManagement::Entity entity) {
	if (!entity.IsValid() || entity.IsNull() || entity.IsNull()) {
		return;
	}

	ImGui::PushID("SkyLight");
	ImGui::PushID((int)entity.GetHandle());

	auto &skyLight = entity.GetComponent<Aquila::SceneManagement::Components::SkyLightComponent>();

	std::vector<ComponentMenuAction> actions = { ComponentMenuAction(
		ComponentMenuAction::REMOVE_COMPONENT, ICON_LC_TRASH_2, "Remove Component",
		[entity]() { entity.RemoveComponent<Aquila::SceneManagement::Components::SkyLightComponent>(); }) };

	const bool headerOpen = DrawComponentHeader(ICON_LC_SUN, "SKYLIGHT", "SkyLightMenu", actions);

	if (headerOpen) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

		ImGui::Indent(8);
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "General");
		ImGui::Spacing();

		if (ImGui::BeginTable("SkyLightGeneral", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Active");

			ImGui::TableNextColumn();
			bool active = skyLight.IsActive();
			if (ImGui::Checkbox("##Active", &active)) {
				skyLight.SetActive(active);
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Lighting");
		ImGui::Spacing();

		if (ImGui::BeginTable("SkyLightLighting", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Intensity");

			ImGui::TableNextColumn();
			f32 intensity = skyLight.GetIntensity();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::SliderFloat("##Intensity", &intensity, 0.0f, 5.0f)) {
				skyLight.SetIntensity(intensity);
			}

			if (skyLight.GetHDRTexture() != nullptr) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Level of Detail");

				ImGui::TableNextColumn();
				f32 lod = skyLight.GetSkyboxLOD();
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::SliderFloat("##LOD", &lod, 0, skyLight.GetHDRTexture()->GetMipLevels())) {
					skyLight.SetSkyboxLOD(lod);
				}
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Tint");

			ImGui::TableNextColumn();
			vec3 tint = skyLight.GetTint();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::ColorEdit3("##Tint", &tint.x)) {
				skyLight.SetTint(tint);
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Render Skybox");

			ImGui::TableNextColumn();
			bool renderSkybox = skyLight.ShouldRenderSkybox();
			if (ImGui::Checkbox("##RenderSkybox", &renderSkybox)) {
				skyLight.SetRenderSkybox(renderSkybox);
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Environment");
		ImGui::Spacing();

		if (ImGui::BeginTable("SkyLightEnvironment", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("HDR Texture");

			ImGui::TableNextColumn();
			ImGuiUtils::DragDrop::AssetDropZone(
				"HDR Texture", "HDR Texture", "None", "HDR_ASSET",
				[&](const char *path) {
					auto &assetManager = m_App.GetAssetManager();
					if (auto hdrTexture = assetManager.TryGetTexture(path)) {
						skyLight.SetHDRTexture(hdrTexture);
					}
				},
				nullptr);

			ImGui::EndTable();
		}

		if (skyLight.IsDirty()) {
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f),
							   ICON_LC_TRIANGLE_ALERT " Environment will rebake next frame");
		}

		ImGui::Spacing();
		ImGui::Unindent(8);
		ImGui::PopStyleVar(2);
	}

	ImGui::Spacing();

	ImGui::PopID();
	ImGui::PopID();
}

void PropertiesPanel::DrawComponent_Metadata(Aquila::SceneManagement::Entity entity) {
	if (!entity.IsValid() || entity.IsNull()) {
		return;
	}
	ImGui::PushID("Metadata");
	ImGui::PushID((int)entity.GetHandle());

	auto &meta = entity.GetComponent<Aquila::SceneManagement::Components::MetadataComponent>();
	char buffer[256];

#ifdef AQUILA_PLATFORM_WINDOWS
	strncpy_s(buffer, meta.GetName().c_str(), sizeof(buffer));
#elif defined(AQUILA_PLATFORM_LINUX)
	strncpy(buffer, meta.Name.c_str(), sizeof(buffer));
	buffer[sizeof(buffer) - 1] = '\0';
#endif

	const bool headerOpen = DrawComponentHeader(ICON_LC_FILE_CODE_2, "ACTOR", "MetadataMenu");

	if (headerOpen) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

		ImGui::Indent(8);
		ImGui::Spacing();

		if (ImGui::BeginTable("MetadataTable", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Display Name");

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::InputText("##Name", buffer, sizeof(buffer))) {
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					meta.SetName(std::string(buffer));
				}
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushFont(UI::FontManager::Get().GetFont14());
		ImGui::TextDisabled("UUID: %s", meta.GetId().ToString().c_str());
		ImGui::PopFont();

		ImGui::Spacing();
		ImGui::Unindent(8);
		ImGui::PopStyleVar(2);
	}
	ImGui::Spacing();

	ImGui::PopID();
	ImGui::PopID();
}

void PropertiesPanel::DrawComponent_Transform(Aquila::SceneManagement::Entity entity) {
	if (!entity.IsValid() || entity.IsNull()) {
		return;
	}
	ImGui::PushID("Transform");
	ImGui::PushID((int)entity.GetHandle());

	auto &transform = entity.GetComponent<Aquila::SceneManagement::Components::TransformComponent>();
	auto position = transform.GetLocalPosition();
	auto rotation = transform.GetLocalRotation();
	auto scale = transform.GetLocalScale();

	std::vector<ComponentMenuAction> actions = {
		ComponentMenuAction(ComponentMenuAction::RESET, ICON_LC_REFRESH_CW, "Reset Transform",
							[&transform]() {
								transform.SetLocalPosition(vec3(0.0f));
								transform.SetLocalRotation(quaternion(1.0f, 0.0f, 0.0f, 0.0f));
								transform.SetLocalScale(vec3(1.0f));
							}),
	};

	const bool headerOpen = DrawComponentHeader(ICON_LC_MOVE_3D, "TRANSFORM", "TransformMenu", actions);

	if (headerOpen) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

		ImGui::Indent(8);
		ImGui::Spacing();

		bool changed = false;

		auto DrawVector3Property = [](const char *label, vec3 &values, f32 resetValue = 0.0f) -> bool {
			ImGui::PushID(label);
			bool modified = false;

			if (ImGui::BeginTable("Vector3Table", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", label);

				ImGui::TableNextColumn();

				constexpr const char *axisLabels[] = { "X", "Y", "Z" };
				constexpr ImVec4 axisColors[] = { { 0.85f, 0.15f, 0.15f, 1.0f },
												  { 0.15f, 0.85f, 0.15f, 1.0f },
												  { 0.15f, 0.45f, 0.95f, 1.0f } };

				const f32 availWidth = ImGui::GetContentRegionAvail().x;
				constexpr f32 buttonSize = 18.0f;
				constexpr f32 spacing = 4.0f;
				const f32 inputWidth = (availWidth - (3 * buttonSize) - (2 * spacing)) / 3.0f;

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 4));

				for (int i = 0; i < 3; i++) {
					ImGui::PushID(i);
					ImGui::PushStyleColor(ImGuiCol_Button, axisColors[i]);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(axisColors[i].x * 1.2f, axisColors[i].y * 1.2f,
																		 axisColors[i].z * 1.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, axisColors[i]);

					if (ImGui::Button(axisLabels[i], ImVec2(buttonSize, 0))) {
						values[i] = resetValue;
						modified = true;
					}
					ImGui::PopStyleColor(3);

					ImGui::SameLine(0.0f, 0.0f);
					ImGui::SetNextItemWidth(inputWidth);
					modified |= ImGui::DragFloat("##value", &values[i], 0.1f, -FLT_MAX, FLT_MAX, "%.2f");

					if (i < 2) {
						ImGui::SameLine(0.0f, spacing);
					}

					ImGui::PopID();
				}

				ImGui::PopStyleVar();
				ImGui::EndTable();
			}

			ImGui::PopID();
			return modified;
		};

		changed |= DrawVector3Property("Location", position);
		ImGui::Spacing();

		vec3 eulerRotation = glm::degrees(glm::eulerAngles(rotation));
		if (DrawVector3Property("Rotation", eulerRotation)) {
			rotation = glm::quat(glm::radians(eulerRotation));
			changed = true;
		}
		ImGui::Spacing();

		changed |= DrawVector3Property("Scale", scale, 1.0f);

		if (changed) {
			transform.SetLocalPosition(position);
			transform.SetLocalRotation(rotation);
			transform.SetLocalScale(scale);
		}

		ImGui::Spacing();
		ImGui::Unindent(8);
		ImGui::PopStyleVar(2);
	}
	ImGui::Spacing();

	ImGui::PopID();
	ImGui::PopID();
}

void PropertiesPanel::DrawComponent_Mesh(Aquila::SceneManagement::Entity entity) {
	if (!entity.IsValid() || entity.IsNull()) {
		return;
	}

	ImGui::PushID("Mesh");
	ImGui::PushID((int)entity.GetHandle());

	auto &component = entity.GetComponent<Aquila::SceneManagement::Components::MeshComponent>();

	std::vector<ComponentMenuAction> actions = { ComponentMenuAction(
		ComponentMenuAction::REMOVE_COMPONENT, ICON_LC_TRASH_2, "Remove Component",
		[entity]() { entity.RemoveComponent<Aquila::SceneManagement::Components::MeshComponent>(); }) };

	const bool headerOpen = DrawComponentHeader(ICON_LC_GRID_3X3, "STATIC MESH", "MeshMenu", actions);

	if (headerOpen) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

		ImGui::Indent(8);
		ImGui::Spacing();

		if (ImGui::BeginTable("MeshTable", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Static Mesh");

			ImGui::TableNextColumn();
			const std::string meshLabel =
				(component.data != nullptr && !component.data->GetPath().empty()) ? component.data->GetPath() : "";

			ImGuiUtils::DragDrop::AssetDropZone(
				"Mesh", "Mesh Asset", meshLabel, "MESH_ASSET",
				[&](const char *path) {
					auto &assetManager = m_App.GetAssetManager();
					if (auto mesh = assetManager.LoadMesh(path)) {
						component.SetMesh(mesh);
						AQUILA_LOG_INFO("Mesh loaded: {}", path);
					}
				},
				[&]() {
					component.SetMesh(nullptr);
					component.ClearMaterials();
				});

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Materials");
		ImGui::Spacing();

		uint32 materialSlotCount = component.GetMaterialSlotCount();
		if (materialSlotCount == 0)
			materialSlotCount = 1;

		for (uint32 i = 0; i < materialSlotCount; ++i) {
			ImGui::PushID(i);

			if (ImGui::BeginTable("MaterialSlot", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();

				if (materialSlotCount > 1) {
					ImGui::Text("Material [%u]", i);
				} else {
					ImGui::Text("Material");
				}

				ImGui::TableNextColumn();

				std::string displayLabel;
				if (component.HasMaterialAsset(i)) {
					displayLabel = component.GetMaterialAssetPath(i);
				} else if (component.HasMaterial(i)) {
					auto mat = component.GetMaterial(i);
					displayLabel = mat ? mat->name : "None";
				} else {
					displayLabel = "None";
				}

				ImGuiUtils::DragDrop::AssetDropZone(("Material_" + std::to_string(i)).c_str(), "Material Asset",
													displayLabel, "MATERIAL_ASSET",
													[&, i](const char *path) {
														auto &assetManager = m_App.GetAssetManager();
														auto materialInstance = assetManager.LoadMaterial(path);
														if (materialInstance) {
															m_App.GetMaterialSystem().AllocateDescriptorsForMaterial(
																materialInstance, assetManager.GetActiveScene());

															component.SetMaterialAsset(path, i);
															component.SetMaterial(materialInstance, i);
															AQUILA_LOG_INFO("Material loaded for slot {}: {}", i, path);
														} else {
															AQUILA_LOG_ERROR("Failed to load material: {}", path);
														}
													},
													[&, i]() {
														component.materials.erase(i);
														component.materialAssetPaths.erase(i);
														AQUILA_LOG_DEBUG("Material cleared from slot {}", i);
													});

				ImGui::EndTable();
			}

			if (component.HasMaterial(i)) {
				auto material = component.GetMaterial(i);
				if (material) {
					ImGui::Indent(8);
					DrawInlineMaterialProperties(material, i);
					ImGui::Unindent(8);
				} else {
					AQUILA_LOG_CRITICAL("Material is null or whatever");
				}
			}

			ImGui::Spacing();
			ImGui::PopID();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Rendering");
		ImGui::Spacing();

		if (ImGui::BeginTable("MeshRenderSettings", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Cast Shadows");

			ImGui::TableNextColumn();
			if (component.data != nullptr) {
				ImGui::Checkbox("##CastShadows", &component.castShadows);
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Receive Shadows");

			ImGui::TableNextColumn();
			if (component.data != nullptr) {
				ImGui::Checkbox("##ReceiveShadows", &component.receiveShadows);
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Unindent(8);
		ImGui::PopStyleVar(2);
	}

	ImGui::Spacing();
	ImGui::PopID();
	ImGui::PopID();
}

void PropertiesPanel::DrawInlineMaterialProperties(Ref<Aquila::Graphics::Material::Material> material,
												   uint32 slotIndex) {
	if (!material) {
		AQUILA_LOG_CRITICAL("Material no good muchachos");
		return;
	}

	auto &assetManager = m_App.GetAssetManager();

	ImGui::PushID(("MatSlot_" + std::to_string(slotIndex)).c_str());

	// Show shader info
	if (material->GetTemplate() && material->GetTemplate()->shader) {
		auto shader = material->GetTemplate()->shader;
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		ImGui::Text(ICON_LC_CODE " Shader: %s", shader->m_Name.c_str());
		if (!material->GetShaderAssetPath().empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("(%s)", material->GetShaderAssetPath().c_str());
		}
		ImGui::PopStyleColor();
	} else {
		ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), ICON_LC_TRIANGLE_ALERT " No shader");
	}

	// Property editor
	if (ImGui::TreeNodeEx("Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto properties = material->GetAllProperties();

		if (ImGui::BeginTable("InlineMaterialProps", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			for (auto &[name, prop] : properties) {
				if (!prop.m_IsEditable || prop.m_Name.empty())
					continue;

				ImGui::PushID(name.c_str());
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("%s", prop.m_DisplayName.c_str());

				ImGui::TableNextColumn();

				switch (prop.m_Type) {
				case Aquila::Graphics::Material::ParameterType::Float: {
					f32 val = std::get<f32>(prop.m_Value);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::SliderFloat("##val", &val, prop.m_MinValue, prop.m_MaxValue, "%.3f")) {
						material->SetParameter(name, val);
					}
					break;
				}
				case Aquila::Graphics::Material::ParameterType::Color: {
					vec4 color = std::get<vec4>(prop.m_Value);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::ColorEdit4("##val", glm::value_ptr(color),
										  ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaBar)) {
						material->SetParameter(name, color);
					}
					break;
				}
				case Aquila::Graphics::Material::ParameterType::Texture2D: {
					auto texture = material->GetTexture(name);
					const std::string texLabel =
						texture && !material->IsFallbackTexture(name, texture) ? "Texture" : "";

					ImGuiUtils::DragDrop::AssetDropZone(
						prop.m_DisplayName.c_str(), "Texture2D", texLabel, "TEXTURE_ASSET",
						[&](const char *path) {
							VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
							if (name.find("albedo") != std::string::npos || name.find("Albedo") != std::string::npos ||
								name.find("diffuse") != std::string::npos ||
								name.find("baseColor") != std::string::npos) {
								format = VK_FORMAT_R8G8B8A8_SRGB;
							}
							if (auto newTexture = assetManager.LoadTexture(path, format)) {
								material->SetParameter(name, newTexture);
								AQUILA_LOG_INFO("Texture loaded: {}", path);
							}
						},
						[&]() { material->SetParameter(name, Ref<Aquila::Graphics::Resources::Texture2D>(nullptr)); });
					break;
				}
				case Aquila::Graphics::Material::ParameterType::Int:
				case Aquila::Graphics::Material::ParameterType::Bool:
				case Aquila::Graphics::Material::ParameterType::Vec2:
				case Aquila::Graphics::Material::ParameterType::Vec3:
				case Aquila::Graphics::Material::ParameterType::Vec4:
					break;
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

void PropertiesPanel::DrawComponent_Camera(Aquila::SceneManagement::Entity entity) {
	ImGui::PushID("Camera");
	ImGui::PushID((int)entity.GetHandle());

	auto &component = entity.GetComponent<Aquila::SceneManagement::Components::CameraComponent>();

	std::vector<ComponentMenuAction> actions = { ComponentMenuAction(
		ComponentMenuAction::REMOVE_COMPONENT, ICON_LC_TRASH_2, "Remove Component",
		[entity]() { entity.RemoveComponent<Aquila::SceneManagement::Components::CameraComponent>(); }) };

	const bool headerOpen = DrawComponentHeader(ICON_LC_CAMERA, "CAMERA", "CameraMenu", actions);

	if (headerOpen) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

		ImGui::Indent(8);
		ImGui::Spacing();

		if (ImGui::BeginTable("CameraBasic", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Primary Camera");
			ImGui::TableNextColumn();
			ImGui::Checkbox("##Primary", &component.primary);

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (ImGui::BeginTable("CameraProjection", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Projection Mode");
			ImGui::TableNextColumn();
			const char *projTypes[] = { "Perspective", "Orthographic" };
			int projType = component.isOrthographic ? 1 : 0;
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::Combo("##ProjType", &projType, projTypes, 2)) {
				component.isOrthographic = (projType == 1);
			}

			ImGui::EndTable();
		}

		ImGui::Spacing();

		if (component.isOrthographic) {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Orthographic Settings");
			ImGui::Spacing();

			if (ImGui::BeginTable("CameraOrtho", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Left");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::DragFloat("##OrthoLeft", &component.orthoLeft, 0.1f, -100.0f, 0.0f);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Right");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::DragFloat("##OrthoRight", &component.orthoRight, 0.1f, 0.0f, 100.0f);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Top");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::DragFloat("##OrthoTop", &component.orthoTop, 0.1f, 0.0f, 100.0f);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Bottom");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::DragFloat("##OrthoBottom", &component.orthoBottom, 0.1f, -100.0f, 0.0f);

				ImGui::EndTable();
			}
		} else {
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Perspective Settings");
			ImGui::Spacing();

			if (ImGui::BeginTable("CameraPerspective", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Field of View");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##FOV", &component.fov, 1.0f, 179.0f, "%.1f°");

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Aspect Ratio");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-60);
				ImGui::DragFloat("##AspectRatio", &component.aspectRatio, 0.01f, 0.1f, 10.0f, "%.3f");
				ImGui::SameLine();
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
				if (ImGui::Button("16:9", ImVec2(0, 0)))
					component.aspectRatio = 16.0f / 9.0f;
				ImGui::PopStyleVar();

				ImGui::EndTable();
			}
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Clipping Planes");
		ImGui::Spacing();

		if (ImGui::BeginTable("CameraClipping", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Near Plane");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat("##NearPlane", &component.nearPlane, 0.01f, 0.001f, 10.0f, "%.3f");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Far Plane");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::DragFloat("##FarPlane", &component.farPlane, 1.0f, 1.0f, 10000.0f, "%.1f");

			ImGui::EndTable();
		}

		ImGui::Spacing();
		ImGui::Unindent(8);
		ImGui::PopStyleVar(2);
	}
	ImGui::Spacing();

	ImGui::PopID();
	ImGui::PopID();
}

void PropertiesPanel::DrawComponent_Light(Aquila::SceneManagement::Entity entity) {
	if (!entity.IsValid() || entity.IsNull()) {
		return;
	}

	ImGui::PushID("Light");
	ImGui::PushID((int)entity.GetHandle());

	auto &component = entity.GetComponent<Aquila::SceneManagement::Components::LightComponent>();

	std::vector<ComponentMenuAction> actions = { ComponentMenuAction(
		ComponentMenuAction::REMOVE_COMPONENT, ICON_LC_TRASH_2, "Remove Component",
		[entity]() { entity.RemoveComponent<Aquila::SceneManagement::Components::LightComponent>(); }) };

	const bool headerOpen = DrawComponentHeader(ICON_LC_LIGHTBULB, "LIGHT", "LightMenu", actions);

	if (headerOpen) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 4));

		ImGui::Indent(8);
		ImGui::Spacing();

		if (ImGui::BeginTable("LightBasic", 2, ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Light Type");
			ImGui::TableNextColumn();
			const char *lightTypes[] = { "Point Light", "Directional Light", "Spot Light" };
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::Combo("##LightType", &reinterpret_cast<int &>(component.m_Type), lightTypes, 3);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Is Active");
			ImGui::TableNextColumn();
			ImGui::Checkbox("##Active", &component.m_IsActive);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Light Color");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::ColorEdit3("##Color", glm::value_ptr(component.m_Color), ImGuiColorEditFlags_NoLabel);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("Intensity");
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::SliderFloat("##Intensity", &component.m_Intensity, 0.0f, 10.0f);

			ImGui::EndTable();
		}

		if (component.m_Type == Aquila::SceneManagement::Components::LightComponent::Type::Spot) {
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Spot Light Settings");
			ImGui::Spacing();

			if (ImGui::BeginTable("LightSpot", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Inner Cone");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##InnerCone", &component.m_InnerConeAngle, 0.0f, 90.0f, "%.1f°");

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Outer Cone");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##OuterCone", &component.m_OuterConeAngle, 0.0f, 90.0f, "%.1f°");

				ImGui::EndTable();
			}
		} else if (component.m_Type == Aquila::SceneManagement::Components::LightComponent::Type::Directional) {
			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Directional Light Settings");
			ImGui::Spacing();

			if (ImGui::BeginTable("LightDirectional", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Direction");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat3("##Direction", glm::value_ptr(component.m_Direction), -1.0f, 1.0f);

				ImGui::EndTable();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Shadow Quality");
			ImGui::Spacing();

			auto &shadowSettings = component.GetShadowSettings();

			if (ImGui::BeginTable("LightShadows", 2, ImGuiTableFlags_SizingStretchProp)) {
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Light Size");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##LightSize", &shadowSettings.lightSize, 0.1f, 5.0f, "%.1f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Controls shadow softness. Higher = softer shadows");

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Shadow Bias");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##ShadowBias", &shadowSettings.shadowBias, 0.0001f, 0.005f, "%.4f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Base depth bias to prevent shadow acne");

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Normal Bias");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##NormalBias", &shadowSettings.normalBias, 0.0f, 5.0f, "%.2f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Normal-based bias multiplier");

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("PCF Samples");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				const char *sampleCounts[] = { "3", "9", "25" };
				int currentSample = shadowSettings.pcfSamples == 3 ? 0 : (shadowSettings.pcfSamples == 9 ? 1 : 2);
				if (ImGui::Combo("##PCFSamples", &currentSample, sampleCounts, 3)) {
					shadowSettings.pcfSamples = (currentSample == 0) ? 3 : (currentSample == 1 ? 9 : 25);
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Number of PCF samples. Higher = smoother but slower");

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Cascade Lambda");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderFloat("##CascadeLambda", &shadowSettings.cascadeSplitLambda, 0.0f, 1.0f, "%.2f");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("0.0 = uniform splits, 1.0 = logarithmic splits");

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Blocker Samples");
				ImGui::TableNextColumn();
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::SliderInt("##BlockerSamples", &shadowSettings.blockerSearchSamples, 8, 32);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Samples for contact hardening. Higher = better quality but slower");

				ImGui::EndTable();
			}
		}

		ImGui::Spacing();
		ImGui::Unindent(8);
		ImGui::PopStyleVar(2);
	}
	ImGui::Spacing();
	ImGui::PopID();
	ImGui::PopID();
}

void PropertiesPanel::DrawComponent_Material(Aquila::SceneManagement::Entity entity) {
	// Intentionally empty – materials are now bound via the mesh component's material slots
	// or via the asset inspector when you click a .aqmat in the content browser.
}

// ═════════════════════════════════════════════════════════════════════════════
// EVENT HANDLING
// ═════════════════════════════════════════════════════════════════════════════
void PropertiesPanel::OnEvent(Aquila::Events::Event &event) {
	Aquila::Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Aquila::Events::EntitySelectedEvent>([this](Aquila::Events::EntitySelectedEvent &event) {
		auto *scene = m_App.GetAssetManager().GetActiveScene();
		if (scene) {
			m_SelectedEntity = event.GetEntity();
			// Entity takes priority – clear asset selection
			m_SelectedAssetPath.clear();
			m_SelectedAssetExtension.clear();
		}
		return false;
	});

	dispatcher.Dispatch<Aquila::Events::EntityDeletedEvent>([this](Aquila::Events::EntityDeletedEvent &event) {
		if (m_SelectedEntity == event.GetEntity()) {
			m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
		}
		return false;
	});

	dispatcher.Dispatch<Aquila::Events::AssetSelectedEvent>([this](Aquila::Events::AssetSelectedEvent &event) {
		if (event.GetPath().empty()) {
			m_SelectedAssetPath.clear();
			m_SelectedAssetExtension.clear();
		} else {
			m_SelectedAssetPath = event.GetPath();
			m_SelectedAssetExtension = event.GetExtension();
			m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
		}
		return false;
	});
}

} // namespace Editor
