#include "UI/Panels/Hierarchy.h"

#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Core/Application.h"
#include "Aquila/Events/SceneEvent.h"
#include "Aquila/Graphics/Core/Renderer.h"
#include "Aquila/Scene/Components/CameraComponent.h"
#include "Aquila/Scene/Components/LightComponent.h"
#include "Aquila/Scene/Components/MeshComponent.h"
#include "Aquila/Scene/Components/MetadataComponent.h"
#include "Aquila/Scene/Components/SceneNodeComponent.h"
#include "Aquila/Scene/Components/TransformComponent.h"
#include "Aquila/Scene/Entity.h"
#include "Aquila/Scene/EntityManager.h"
#include "Aquila/Scene/Scene.h"
#include "Aquila/Scene/SceneManager.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <lucide.h>

namespace Editor {

static ImGuiTextFilter s_HierarchyFilter;
static Aquila::SceneManagement::Entity s_DoubleClicked = Aquila::SceneManagement::Entity::Null();
static Aquila::SceneManagement::Entity s_HadRecentDroppedEntity = Aquila::SceneManagement::Entity::Null();

SceneHierarchyPanel::SceneHierarchyPanel(Aquila::Core::Application &app) : Layer("SceneHierarchy"), m_App(app) {
	AQUILA_LOG_INFO("SceneHierarchyPanel created");
}

void SceneHierarchyPanel::OnAttach() {
	AQUILA_LOG_INFO("SceneHierarchyPanel attached");
	m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
}

void SceneHierarchyPanel::OnDetach() {
	AQUILA_LOG_INFO("SceneHierarchyPanel detached");
}

void SceneHierarchyPanel::OnUpdate(f32 deltaTime) {}

void SceneHierarchyPanel::OnImGuiRender() {
	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (scene == nullptr) {
		return;
	}
	auto *entityManager = scene->GetEntityManager();

	auto flags = ImGuiWindowFlags_NoCollapse;

	if (ImGui::Begin((std::string(ICON_LC_LAYERS_2) + " Scene Hierarchy").c_str(), nullptr, flags)) {
		ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImGui::GetStyleColorVec4(ImGuiCol_TabActive));

		// Search bar
		ImGui::TextUnformatted(ICON_LC_SEARCH);
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_TitleBg));
		ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(ImGuiCol_Border));
		s_HierarchyFilter.Draw("##HierarchyFilter", ImGui::GetContentRegionAvail().x - ImGui::GetStyle().IndentSpacing);
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();

		// Search placeholder text
		if (!s_HierarchyFilter.IsActive() && !ImGui::IsItemActive()) {
			ImVec2 pos = ImGui::GetItemRectMin();
			ImVec2 size = ImGui::GetItemRectSize();
			ImDrawList *drawList = ImGui::GetWindowDrawList();
			ImVec2 textPos = pos;
			textPos.x += ImGui::GetStyle().FramePadding.x + 4.0F;
			textPos.y += (size.y - ImGui::GetFontSize()) * 0.5F;
			drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), "Search...");
		}

		ImGui::PopStyleColor();
		ImGui::Unindent();

		// Right-click on empty space
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
			HandleEntitySelection(Aquila::SceneManagement::Entity::Null());
			ImGui::OpenPopup("SceneHierarchyPopup");
		}

		DrawPopupMenu();

		ImGui::Indent();

		entityManager->ForEach<Aquila::SceneManagement::Components::SceneNodeComponent>(
			[this](Aquila::SceneManagement::Entity entity,
				   Aquila::SceneManagement::Components::SceneNodeComponent &node) {
				if (node.Parent.IsNull()) {
					DrawEntityTree(entity);
				}
			});

		// Drag-drop to root
		ImVec2 minSpace = ImGui::GetWindowContentRegionMin();
		ImVec2 maxSpace = ImGui::GetWindowContentRegionMax();
		minSpace.x += ImGui::GetWindowPos().x + 1.0F;
		minSpace.y += ImGui::GetWindowPos().y + 1.0F;
		maxSpace.x += ImGui::GetWindowPos().x - 1.0F;
		maxSpace.y += ImGui::GetWindowPos().y - 1.0F;

		if (const ImRect windowRect = { minSpace, maxSpace };
			ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetCurrentWindow()->ID)) {
			const ImGuiPayload *payload =
				ImGui::AcceptDragDropPayload("Drag_Entity", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
			if (payload != nullptr) {
				const Aquila::SceneManagement::Entity droppedEntity =
					*static_cast<Aquila::SceneManagement::Entity *>(payload->Data);

				if (const Aquila::SceneManagement::Entity entity(droppedEntity.GetHandle(), scene); entity.IsValid()) {
					const auto *node =
						entity.TryGetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>();
					entityManager->RemoveChild(node->Parent, entity);
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Delete key
		if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete)) {
			if (!m_SelectedEntity.IsNull()) {
				DeleteEntity(m_SelectedEntity);
			}
		}
	}
	ImGui::End();
}

void SceneHierarchyPanel::DrawEntityTree(Aquila::SceneManagement::Entity entityHandle) {
	if (!entityHandle.IsValid()) {
		return;
	}

	const auto *scene = m_App.GetAssetManager().GetActiveScene();
	auto *entityManager = scene->GetEntityManager();
	if (scene == nullptr) {
		AQUILA_LOG_CRITICAL("Engine found no scene!");
		return;
	}
	if (entityManager->IsRegistryEmpty()) {
		AQUILA_LOG_DEBUG("Registry empty, exiting");
		return;
	}

	if (!entityHandle.IsValid() ||
		!entityHandle.HasAllComponents<Aquila::SceneManagement::Components::MetadataComponent,
									   Aquila::SceneManagement::Components::TransformComponent>()) {
		AQUILA_LOG_CRITICAL("Registry not valid!");
		return;
	}

	auto &data = entityHandle.GetComponent<Aquila::SceneManagement::Components::MetadataComponent>();
	const auto &node = entityHandle.GetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>();

	// Frustum culling for performance
	if (const bool visible =
			ImGui::IsRectVisible(ImVec2(ImGui::GetContentRegionMax().x, ImGui::GetTextLineHeightWithSpacing()));
		!visible) {
		ImGui::NewLine();
		return;
	}

	// Filter check
	bool show = true;
	if (s_HierarchyFilter.IsActive()) {
		if (!s_HierarchyFilter.PassFilter(data.GetName().c_str())) {
			show = false;
		}
	}

	if (show) {
		ImGui::PushID(data.GetId().ToString().c_str());

		const bool noChildren = node.Children.empty();

		ImGuiTreeNodeFlags nodeFlags = 0;
		if (m_SelectedEntity == entityHandle) {
			nodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding |
					 ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

		if (noChildren) {
			nodeFlags |= ImGuiTreeNodeFlags_Leaf;
		}

		const bool active = data.IsVisible();
		if (!active) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
		}

		const bool doubleClicked = (entityHandle == s_DoubleClicked);

		if (doubleClicked) {
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1.0f, 2.0f });
		}

		if (s_HadRecentDroppedEntity == entityHandle) {
			ImGui::SetNextItemOpen(true);
			s_HadRecentDroppedEntity = Aquila::SceneManagement::Entity::Null();
		}

		const char *icon = ICON_LC_BOX;

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

		const bool nodeOpen = ImGui::TreeNodeEx(
			reinterpret_cast<void *>(static_cast<intptr_t>(entityHandle.GetHandle())), nodeFlags, "%s", icon);

		// Drag source
		if (ImGui::BeginDragDropSource()) {
			const auto selected = entityHandle;
			ImGui::TextUnformatted(data.GetName().c_str());
			ImGui::SetDragDropPayload("Drag_Entity", &selected, sizeof(Aquila::SceneManagement::Entity));
			ImGui::EndDragDropSource();
		}

		// Selection handling
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && ImGui::IsItemHovered() && !ImGui::IsItemToggledOpen()) {
			HandleEntitySelection(entityHandle);
		} else if (s_DoubleClicked == entityHandle && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
				   !ImGui::IsItemHovered()) {
			s_DoubleClicked = Aquila::SceneManagement::Entity::Null();
		}

		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && ImGui::IsItemHovered()) {
			s_DoubleClicked = entityHandle;
		}

		ImGui::PopStyleColor();
		ImGui::SameLine();

		// Entity name / rename input
		if (!doubleClicked) {
			ImGui::TextUnformatted(data.GetName().c_str());
		} else {
			static char nameBuffer[256];
			strcpy_s(nameBuffer, sizeof(nameBuffer), data.GetName().c_str());

			ImGui::PushItemWidth(-1);
			if (ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
				data.SetName(nameBuffer);
				s_DoubleClicked = Aquila::SceneManagement::Entity::Null();
			}
			if (doubleClicked) {
				ImGui::PopStyleVar();
			}
		}

		if (!active) {
			ImGui::PopStyleColor();
		}

		// Context menu
		if (ImGui::BeginPopupContextItem(data.GetId().ToString().c_str())) {
			HandleEntitySelection(entityHandle);
			DrawContextMenu();
			ImGui::EndPopup();
		}

		// Drop target
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("Drag_Entity")) {
				if (const Aquila::SceneManagement::Entity droppedEntity =
						*static_cast<Aquila::SceneManagement::Entity *>(payload->Data);
					droppedEntity != entityHandle) {
					auto *scene = m_App.GetAssetManager().GetActiveScene();
					auto *const entityManager = scene->GetEntityManager();
					const Aquila::SceneManagement::Entity childEntity(droppedEntity.GetHandle(), scene);

					if (const Aquila::SceneManagement::Entity parentEntity(entityHandle.GetHandle(), scene);
						childEntity.IsValid() && parentEntity.IsValid()) {
						entityManager->AttachTo(parentEntity, childEntity);
						s_HadRecentDroppedEntity = entityHandle;
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Visibility toggle button
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - (ImGui::CalcTextSize(ICON_LC_EYE).x * 2.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));
		if (ImGui::Button(active ? ICON_LC_EYE : ICON_LC_EYE_CLOSED)) {
			data.SetVisible(!active);
			Aquila::Events::EntityVisibilityToggle event(entityHandle, data.IsVisible());
			m_App.OnEvent(event);
			ToggleVisibility(entityHandle, data.IsVisible());
		}
		ImGui::PopStyleColor();

		if (!nodeOpen) {
			ImGui::PopID();
			return;
		}

		// Tree lines
		const ImColor TreeLineColor = ImColor(128, 128, 128, 128);
		const f32 SmallOffsetX = 6.0f;
		ImDrawList *drawList = ImGui::GetWindowDrawList();

		ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
		verticalLineStart.x += SmallOffsetX;
		ImVec2 verticalLineEnd = verticalLineStart;

		// Draw children
		if (!noChildren) {
			for (auto child : node.Children) {
				if (!child.IsValid() ||
					!child.HasAllComponents<Aquila::SceneManagement::Components::MetadataComponent,
											Aquila::SceneManagement::Components::SceneNodeComponent>()) {
					continue;
				}

				f32 HorizontalTreeLineSize = 20.0f;
				auto currentPos = ImGui::GetCursorScreenPos();
				ImGui::Indent(10.0f);

				DrawEntityTree(child);
				ImGui::Unindent(10.0f);

				const ImRect childRect =
					ImRect(currentPos, ImVec2(currentPos.x, currentPos.y + ImGui::GetFontSize() +
																(ImGui::GetStyle().FramePadding.y * 2)));
				const f32 midpoint = (childRect.Min.y + childRect.Max.y) * 0.5f;
				drawList->AddLine(ImVec2(verticalLineStart.x, midpoint),
								  ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
				verticalLineEnd.y = midpoint;
			}
		}

		drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

		ImGui::TreePop();
		ImGui::PopID();
	}
}

void SceneHierarchyPanel::DrawEntityNode(Aquila::SceneManagement::Entity entity) {
	DrawEntityTree(entity);
}

void SceneHierarchyPanel::DrawContextMenu() {
	if (!m_SelectedEntity.IsNull()) {
		auto *scene = m_App.GetAssetManager().GetActiveScene();
		if (scene == nullptr) {
			return;
		}
		auto *entityManager = scene->GetEntityManager();
		ImGui::SeparatorText("Family");

		if (ImGui::MenuItem(ICON_LC_PLUS " Create empty child entity")) {
			CreateChildEntity(m_SelectedEntity);
		}

		if (scene != nullptr) {
			if (m_SelectedEntity.IsValid() && !m_SelectedEntity.IsNull() && m_SelectedEntity.Exists() &&
				(m_SelectedEntity.TryGetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>() !=
				 nullptr)) {
				auto *node =
					m_SelectedEntity.TryGetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>();
				if (!node->Parent.IsNull()) {
					if (ImGui::MenuItem(ICON_LC_UNLINK " Detach from parent")) {
						entityManager->RemoveChild(node->Parent, m_SelectedEntity);
					}
				}
			}
		}

		// if (registry.valid(m_SelectedEntity.GetHandle()) &&
		// 	registry.all_of<Aquila::SceneManagement::Components::SceneNodeComponent>(m_SelectedEntity.GetHandle())) {
		// 	auto &node =
		// 		registry.get<Aquila::SceneManagement::Components::SceneNodeComponent>(m_SelectedEntity.GetHandle());

		// 	if (!node.Parent.IsNull()) {
		// 		if (ImGui::MenuItem(ICON_LC_UNLINK " Detach from parent")) {
		// 			auto entityManager = scene->GetEntityManager();
		// 			Aquila::SceneManagement::Entity entity(m_SelectedEntity.GetHandle(), scene);
		// 			if (entity.IsValid()) {
		// 				entityManager->RemoveChild()
		// 			}
		// 		}
		// 	}
		// }

		ImGui::SeparatorText("Create Objects");

		if (ImGui::MenuItem(ICON_LC_PLUS " Create empty entity")) {
			CreateEntity(Aquila::SceneManagement::EntityPreset::Empty);
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("3D Objects")) {
			if (ImGui::MenuItem("Cube")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Cube);
			}
			if (ImGui::MenuItem("Sphere")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Sphere);
			}
			if (ImGui::MenuItem("Cylinder")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Cylinder);
			}
			if (ImGui::MenuItem("Plane")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Plane);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Lights")) {
			if (ImGui::MenuItem("Directional Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::DirectionalLight);
			}
			if (ImGui::MenuItem("Point Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::PointLight);
			}
			if (ImGui::MenuItem("Spot Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::SpotLight);
			}
			if (ImGui::MenuItem("Sky Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::SkyLight);
			}
			if (ImGui::MenuItem("Area Light")) {
				// TODO: Create area light
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Cameras")) {
			if (ImGui::MenuItem("Perspective Camera")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::PerspectiveCamera);
			}
			if (ImGui::MenuItem("Orthographic Camera")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::OrthographicCamera);
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_TRASH " Delete entity")) {
			DeleteEntity(m_SelectedEntity);
		}
	} else {
		if (ImGui::MenuItem(ICON_LC_PLUS " Create empty entity")) {
			CreateEntity(Aquila::SceneManagement::EntityPreset::Empty);
		}

		ImGui::Separator();

		if (ImGui::BeginMenu("3D Objects")) {
			if (ImGui::MenuItem("Cube")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Cube);
			}
			if (ImGui::MenuItem("Sphere")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Sphere);
			}
			if (ImGui::MenuItem("Cylinder")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Cylinder);
			}
			if (ImGui::MenuItem("Plane")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::Plane);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Lights")) {
			if (ImGui::MenuItem("Directional Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::DirectionalLight);
			}
			if (ImGui::MenuItem("Point Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::PointLight);
			}
			if (ImGui::MenuItem("Spot Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::SpotLight);
			}
			if (ImGui::MenuItem("Sky Light")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::SkyLight);
			}
			if (ImGui::MenuItem("Area Light")) {
				// TODO: Create area light
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Cameras")) {
			if (ImGui::MenuItem("Perspective Camera")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::PerspectiveCamera);
			}
			if (ImGui::MenuItem("Orthographic Camera")) {
				CreateEntity(Aquila::SceneManagement::EntityPreset::OrthographicCamera);
			}
			ImGui::EndMenu();
		}
	}
}

void SceneHierarchyPanel::DrawPopupMenu() {
	if (ImGui::BeginPopup("SceneHierarchyPopup", ImGuiWindowFlags_NoMove)) {
		DrawContextMenu();
		ImGui::EndPopup();
	}
}

void SceneHierarchyPanel::HandleEntitySelection(Aquila::SceneManagement::Entity entity) {
	m_SelectedEntity = entity;

	Aquila::Events::EntitySelectedEvent event(entity);
	m_App.OnEvent(event);
}

void SceneHierarchyPanel::CreateEntity(Aquila::SceneManagement::EntityPreset preset) {
	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (scene == nullptr) {
		return;
	}

	auto *entityManager = scene->GetEntityManager();
	auto entity = entityManager->CreateEntity(entityManager->GetDefaultName(preset));

	if (!entity.IsValid()) {
		return;
	}

	switch (preset) {
	case Aquila::SceneManagement::EntityPreset::Empty:
		break;

	case Aquila::SceneManagement::EntityPreset::Cube: {
		auto &meshComponent = entity.AddComponent<Aquila::SceneManagement::Components::MeshComponent>();
		auto mesh = CreateRef<Aquila::Graphics::Resources::Mesh>(m_App.GetDevice(), "procedural_cube");
		mesh->LoadFromData(Aquila::Graphics::Resources::Mesh::GenerateCube(1.0f));
		meshComponent.SetMesh(mesh);

		// ADD DEFAULT MATERIAL
		auto &matSystem = m_App.GetRenderer().GetMaterialSystem();
		auto defaultMat = matSystem.GetLibrary().GetDefaultMaterial();
		meshComponent.SetMaterial(defaultMat, 0);

		break;
	}

	case Aquila::SceneManagement::EntityPreset::Sphere: {
		auto &meshComponent = entity.AddComponent<Aquila::SceneManagement::Components::MeshComponent>();
		auto mesh = CreateRef<Aquila::Graphics::Resources::Mesh>(m_App.GetDevice(), "procedural_sphere");
		mesh->LoadFromData(Aquila::Graphics::Resources::Mesh::GenerateSphere(1.0f, 32, 16));
		meshComponent.SetMesh(mesh);

		auto &matSystem = m_App.GetRenderer().GetMaterialSystem();
		auto defaultMat = matSystem.GetLibrary().GetDefaultMaterial();
		meshComponent.SetMaterial(defaultMat, 0);

		break;
	}

	case Aquila::SceneManagement::EntityPreset::Cylinder: {
		auto &meshComponent = entity.AddComponent<Aquila::SceneManagement::Components::MeshComponent>();
		auto mesh = CreateRef<Aquila::Graphics::Resources::Mesh>(m_App.GetDevice(), "procedural_cylinder");
		mesh->LoadFromData(Aquila::Graphics::Resources::Mesh::GenerateCylinder(1.0f, 2.0f, 32));
		meshComponent.SetMesh(mesh);

		// ADD DEFAULT MATERIAL
		auto &matSystem = m_App.GetRenderer().GetMaterialSystem();
		auto defaultMat = matSystem.GetLibrary().GetDefaultMaterial();
		meshComponent.SetMaterial(defaultMat, 0);

		break;
	}

	case Aquila::SceneManagement::EntityPreset::Plane: {
		auto &meshComponent = entity.AddComponent<Aquila::SceneManagement::Components::MeshComponent>();
		auto mesh = CreateRef<Aquila::Graphics::Resources::Mesh>(m_App.GetDevice(), "procedural_plane");
		mesh->LoadFromData(Aquila::Graphics::Resources::Mesh::GeneratePlane(2.0f, 2.0f, 10, 10));
		meshComponent.SetMesh(mesh);

		// ADD DEFAULT MATERIAL
		auto &matSystem = m_App.GetRenderer().GetMaterialSystem();
		auto defaultMat = matSystem.GetLibrary().GetDefaultMaterial();
		meshComponent.SetMaterial(defaultMat, 0);

		break;
	}

	case Aquila::SceneManagement::EntityPreset::DirectionalLight: {
		auto &component = entity.AddComponent<Aquila::SceneManagement::Components::LightComponent>();
		component.m_Type = Aquila::SceneManagement::Components::LightComponent::Type::Directional;
		break;
	}

	case Aquila::SceneManagement::EntityPreset::PointLight: {
		auto &component = entity.AddComponent<Aquila::SceneManagement::Components::LightComponent>();
		component.m_Type = Aquila::SceneManagement::Components::LightComponent::Type::Point;
		break;
	}

	case Aquila::SceneManagement::EntityPreset::SpotLight: {
		auto &component = entity.AddComponent<Aquila::SceneManagement::Components::LightComponent>();
		component.m_Type = Aquila::SceneManagement::Components::LightComponent::Type::Spot;
		break;
	}

	case Aquila::SceneManagement::EntityPreset::PerspectiveCamera: {
		auto &component = entity.AddComponent<Aquila::SceneManagement::Components::CameraComponent>();
		component.isOrthographic = false;
		break;
	}

	case Aquila::SceneManagement::EntityPreset::OrthographicCamera: {
		auto &component = entity.AddComponent<Aquila::SceneManagement::Components::CameraComponent>();
		component.isOrthographic = true;
		break;
	}

	case Aquila::SceneManagement::EntityPreset::SkyLight: {
		auto &component = entity.AddComponent<Aquila::SceneManagement::Components::SkyLightComponent>();
		component.m_IsActive = true;
		break;
	}

	default:
		break;
	}

	AQUILA_LOG_INFO("Added entity {} with UUID: {}", entity.GetName(), entity.GetUUID().ToString());

	// Dispatch entity created event
	Aquila::Events::EntityCreatedEvent event(entity);
	m_App.OnEvent(event);

	// Select the newly created entity
	HandleEntitySelection(entity);
}

void SceneHierarchyPanel::CreateChildEntity(Aquila::SceneManagement::Entity parent) {
	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (scene == nullptr) {
		return;
	}

	auto *entityManager = scene->GetEntityManager();
	auto entity = entityManager->CreateEntity("Child Entity");

	if (!entity.IsValid()) {
		return;
	}

	if (parent.IsValid()) {
		entityManager->AttachTo(parent, entity);
	}

	AQUILA_LOG_INFO("Created child entity {} under parent", entity.GetName());

	Aquila::Events::EntityCreatedEvent event(entity);
	m_App.OnEvent(event);

	HandleEntitySelection(entity);
}

void SceneHierarchyPanel::DeleteEntity(Aquila::SceneManagement::Entity entity) {
	auto *scene = m_App.GetAssetManager().GetActiveScene();
	if (scene == nullptr || !entity.IsValid()) {
		return;
	}

	auto *node = entity.TryGetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>();
	if (node != nullptr) {
		for (auto &child : node->Children) {
			DeleteEntity(child); // recursively notify + queue children
		}
	}

	AQUILA_LOG_INFO("Deleting entity {}", entity.GetName());

	Aquila::Events::EntityDeletedEvent event(entity);
	m_App.OnEvent(event);

	if (m_SelectedEntity == entity) {
		HandleEntitySelection(Aquila::SceneManagement::Entity::Null());
	}

	scene->GetEntityManager()->QueueForKill(entity);
}
void SceneHierarchyPanel::ToggleVisibility(Aquila::SceneManagement::Entity entity, bool value) {
	if (!entity.IsValid() || !entity.HasAllComponents<Aquila::SceneManagement::Components::MetadataComponent,
													  Aquila::SceneManagement::Components::SceneNodeComponent>()) {
		return;
	}

	auto &node = entity.GetComponent<Aquila::SceneManagement::Components::SceneNodeComponent>();
	if (!node.Children.empty()) {
		for (auto &child : node.Children) {
			auto &meta = child.GetComponent<Aquila::SceneManagement::Components::MetadataComponent>();
			meta.SetVisible(value);
			Aquila::Events::EntityVisibilityToggle event(child, meta.IsVisible());
			m_App.OnEvent(event);
			ToggleVisibility(child, value);
		}
	}
}

void SceneHierarchyPanel::OnEvent(Aquila::Events::Event &event) {
	Aquila::Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Aquila::Events::EntitySelectedEvent>([this](Aquila::Events::EntitySelectedEvent &event) {
		m_SelectedEntity = Aquila::SceneManagement::Entity(event.GetEntity());
		return false;
	});

	dispatcher.Dispatch<Aquila::Events::EntityDeletedEvent>([this](Aquila::Events::EntityDeletedEvent &event) {
		if (m_SelectedEntity == event.GetEntity()) {
			m_SelectedEntity = Aquila::SceneManagement::Entity::Null();
		}
		return false;
	});
}

} // namespace Editor
