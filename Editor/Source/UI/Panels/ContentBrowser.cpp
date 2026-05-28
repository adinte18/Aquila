#include "UI/Panels/ContentBrowser.h"

#include <algorithm>
#include "Aquila/Core/Defines.h"
#include "UI/ImGuiUtils.h"
#include "Aquila/Core/Application.h"
#include "Aquila/Core/Layer.h"
#include "Aquila/Events/SceneEvent.h"
#include "Aquila/Events/AssetEvent.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include "Aquila/Assets/AssetManager.h"
#include "Aquila/Graphics/Shader/ShaderCompiler.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"
#include "lucide.h"
#include "Aquila/Foundation/Profiler.h"
namespace Editor {

ContentBrowserPanel::ContentBrowserPanel(Aquila::Core::Application &app)
	: Layer("ContentBrowser"), m_App(app), m_CurrentPath("assets://") {
	AQUILA_LOG_INFO("ContentBrowserPanel created");
	m_RootNode = FolderNode{ .fullPath = "assets://", .name = "assets://", .loaded = false, .children = {} };
}

void ContentBrowserPanel::OnAttach() {
	AQUILA_LOG_INFO("ContentBrowserPanel attached");

	std::vector<std::string> texturePaths = { ENGINE_RESOURCES_PATH "/folder.png",
											  ENGINE_RESOURCES_PATH "/aquila-logo.png",
											  ENGINE_RESOURCES_PATH "/file.png" };

	auto &assetManager = m_App.GetAssetManager();
	m_Textures.reserve(texturePaths.size());

	for (const auto &path : texturePaths) {
		try {
			if (auto texture = assetManager.LoadTexture(path, VK_FORMAT_R8G8B8A8_UNORM)) {
				m_Textures.push_back(texture);
			} else {
				AQUILA_LOG_WARNING("Failed to load UI texture: {}", path);
			}
		} catch (const std::exception &e) {
			AQUILA_LOG_ERROR("Exception loading UI texture {}: {}", path, e.what());
		}
	}

	if (m_Textures.size() < 3) {
		AQUILA_LOG_ERROR("Failed to load all UI icons!");
		return;
	}

	m_IconsLoaded = true;
	LoadContentBrowserTextures();

	AQUILA_LOG_INFO("ContentBrowser icons loaded successfully");

	RefreshDirectoryCache();
}

void ContentBrowserPanel::OnDetach() {
	AQUILA_LOG_INFO("ContentBrowserPanel detached");
	m_ImagePreviews.clear();
	m_Textures.clear();
	m_DirectoryCache.clear();
}

void ContentBrowserPanel::OnUpdate(f32 deltaTime) {}

void ContentBrowserPanel::OnImGuiRender() {
	PROFILE_SCOPE("ContentBrowser::OnImGuiRender");

	if (!m_IconsLoaded) {
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::Begin(ICON_LC_PACKAGE_OPEN " Content Browser", nullptr,
				 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	ImGui::PopStyleVar();

	ImVec2 windowSize = ImGui::GetContentRegionAvail();

	{
		PROFILE_SCOPE("DrawToolbar");
		DrawToolbar(windowSize);
	}

	const f32 splitterHeight = 4.0f;
	const f32 toolbarHeight = 44.0f;
	const f32 bottomPanelHeight = 35.0f;
	const f32 availableHeight = windowSize.y - toolbarHeight - splitterHeight - bottomPanelHeight;

	{
		PROFILE_SCOPE("DrawFolderTree");
		ImGui::BeginChild("FolderTree", ImVec2(m_FolderPanelWidth, availableHeight), true);
		DrawFolderTree(m_RootNode, m_CurrentPath);
		ImGui::EndChild();
	}

	ImGui::SameLine();
	ImGui::Button("##splitter", ImVec2(splitterHeight, availableHeight));
	if (ImGui::IsItemActive()) {
		m_FolderPanelWidth += ImGui::GetIO().MouseDelta.x;
		m_FolderPanelWidth = std::clamp(m_FolderPanelWidth, 150.0f, 400.0f);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}

	ImGui::SameLine();
	const f32 contentWidth = windowSize.x - m_FolderPanelWidth - splitterHeight - 16.0f;

	{
		PROFILE_SCOPE("DrawContentGrid");
		ImGui::BeginChild("ContentGrid", ImVec2(contentWidth, availableHeight), true, ImGuiWindowFlags_NoScrollbar);
		DrawContentGrid(contentWidth);
		ImGui::EndChild();
	}

	{
		PROFILE_SCOPE("DrawInfoPanel");
		ImGui::BeginChild("InfoPanel", ImVec2(windowSize.x - 8.0f, bottomPanelHeight), true,
						  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		DrawInfoPanel();
		ImGui::EndChild();
	}

	{
		PROFILE_SCOPE("HandleContextMenusAndPopups");
		HandleContextMenus();
		HandlePopups();
	}

	ImGui::End();
}
void ContentBrowserPanel::RefreshDirectoryCache() {
	auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

	auto &cache = m_DirectoryCache[m_CurrentPath];
	cache.entries.clear();
	cache.filteredEntries.clear();
	cache.lastSearchQuery = "\x01";
	cache.needsRefresh = false;

	if (!vfs->Exists(m_CurrentPath) || !vfs->IsDirectory(m_CurrentPath)) {
		return;
	}

	auto entryNames = vfs->ListDirectory(m_CurrentPath);
	cache.entries.reserve(entryNames.size());

	for (const auto &entryName : entryNames) {
		std::string fullPath = m_CurrentPath;
		if (fullPath.back() != '/') {
			fullPath += "/";
		}
		fullPath += entryName;

		DirectoryEntry entry;
		entry.name = entryName;
		entry.fullPath = fullPath;
		entry.isDirectory = vfs->IsDirectory(fullPath);

		if (!entry.isDirectory) {
			entry.extension = GetFileExtension(entryName);
			entry.fileType = GetFileTypeString(fullPath);
		}

		cache.entries.push_back(std::move(entry));
	}

	std::ranges::sort(cache.entries, [](const DirectoryEntry &a, const DirectoryEntry &b) {
		if (a.isDirectory != b.isDirectory) {
			return a.isDirectory;
		}
		return a.name < b.name;
	});

	FilterDirectoryCache();
}

void ContentBrowserPanel::FilterDirectoryCache() {
	auto &cache = m_DirectoryCache[m_CurrentPath];

	std::string searchQuery = m_SearchBuffer;

	if (searchQuery == cache.lastSearchQuery && !cache.filteredEntries.empty()) {
		return;
	}

	cache.lastSearchQuery = searchQuery;
	cache.filteredEntries.clear();

	if (searchQuery.empty()) {
		cache.filteredEntries = cache.entries;
		return;
	}

	std::string lowerSearch = searchQuery;
	std::ranges::transform(lowerSearch, lowerSearch.begin(), [](unsigned char c) { return std::tolower(c); });

	cache.filteredEntries.reserve(cache.entries.size());
	for (const auto &entry : cache.entries) {
		std::string lowerName = entry.name;
		std::ranges::transform(lowerName, lowerName.begin(), [](unsigned char c) { return std::tolower(c); });

		if (lowerName.find(lowerSearch) != std::string::npos) {
			cache.filteredEntries.push_back(entry);
		}
	}
}

void ContentBrowserPanel::DrawToolbar(const ImVec2 &windowSize) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
	ImGui::BeginChild("Toolbar", ImVec2(windowSize.x, 44), true, ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleVar();

	const f32 buttonSize = 28.0f;
	const f32 itemSpacing = 4.0f;

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(itemSpacing, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));

	if (m_CurrentPath != "assets://") {
		if (ImGui::Button(ICON_LC_ARROW_LEFT "##back", ImVec2(buttonSize, buttonSize))) {
			m_CurrentPath = GetParentPath(m_CurrentPath);
			m_SelectedItems.clear();
			RefreshDirectoryCache();
		}
	} else {
		ImGuiUtils::ScopedDisable disable(true);
		ImGui::Button(ICON_LC_ARROW_LEFT "##back", ImVec2(buttonSize, buttonSize));
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Back");
	}

	ImGui::SameLine();

	{
		ImGuiUtils::ScopedDisable disable(true);
		ImGui::Button(ICON_LC_ARROW_RIGHT "##forward", ImVec2(buttonSize, buttonSize));
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Forward");
	}

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();

	if (ImGui::Button(ICON_LC_FOLDER_PLUS "##newfolder", ImVec2(buttonSize, buttonSize))) {
		m_ShowCreateFolderPopup = true;
		ImGui::SetWindowFocus("Create Folder");
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Create Folder");
	}

	ImGui::SameLine();

	if (ImGui::Button(ICON_LC_REFRESH_CW "##refresh", ImVec2(buttonSize, buttonSize))) {
		m_ImagePreviews.clear();
		auto &cache = m_DirectoryCache[m_CurrentPath];
		cache.needsRefresh = true;
		RefreshDirectoryCache();
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Refresh");
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();

	DrawBreadcrumb();

	ImGui::SameLine();

	const f32 searchWidth = 500.0f;
	const f32 viewButtonsWidth = (buttonSize * 2) + itemSpacing;
	const f32 rightSideWidth = searchWidth + viewButtonsWidth + itemSpacing * 3;

	ImGuiUtils::AlignedWidget(
		ImGuiUtils::AlignX::Right, ImGuiUtils::AlignY::Center, ImVec2(itemSpacing, 0),
		ImVec2(rightSideWidth, buttonSize), [&]() {
			ImGuiUtils::AlignedWidget(ImGuiUtils::AlignX::Keep, ImGuiUtils::AlignY::Center, ImVec2(0, 0),
									  ImVec2(searchWidth, buttonSize), [&]() {
										  if (ImGuiUtils::SearchBar("##search", m_SearchBuffer, sizeof(m_SearchBuffer),
																	std::string(ICON_LC_SEARCH " Search...").c_str(),
																	searchWidth)) {
											  FilterDirectoryCache();
										  }
									  });

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button,
								  m_ViewMode == ViewMode::Grid ? ImVec4(0.3f, 0.5f, 0.8f, 1.0f) : ImVec4(0, 0, 0, 0));
			if (ImGui::Button(ICON_LC_GRID_3X3 "##grid", ImVec2(buttonSize, buttonSize))) {
				m_ViewMode = ViewMode::Grid;
			}
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Grid View");
			}

			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button,
								  m_ViewMode == ViewMode::List ? ImVec4(0.3f, 0.5f, 0.8f, 1.0f) : ImVec4(0, 0, 0, 0));
			if (ImGui::Button(ICON_LC_LIST "##list", ImVec2(buttonSize, buttonSize))) {
				m_ViewMode = ViewMode::List;
			}
			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("List View");
			}
		});

	ImGui::PopStyleVar();
	ImGui::EndChild();
}

void ContentBrowserPanel::DrawBreadcrumb() {
	std::vector<std::string> pathSegments;
	std::string currentSegment;

	for (char path : m_CurrentPath) {
		if (path == '/' && !currentSegment.empty()) {
			pathSegments.push_back(currentSegment);
			currentSegment.clear();
		} else if (path != '/') {
			currentSegment += path;
		}
	}

	if (!currentSegment.empty()) {
		pathSegments.push_back(currentSegment);
	}

	const f32 buttonHeight = 28.0f;

	std::string builtPath;
	for (size_t i = 0; i < pathSegments.size(); i++) {
		if (i > 0) {
			ImGui::SameLine();
			ImGui::TextDisabled("/");
			ImGui::SameLine();
		}

		builtPath += pathSegments[i] + "//";

		ImGui::PushID(static_cast<int>(i));

		ImVec2 textSize = ImGui::CalcTextSize(pathSegments[i].c_str());
		ImVec2 buttonSize = ImVec2(textSize.x + 16.0f, buttonHeight);

		if (ImGui::Button(pathSegments[i].c_str(), buttonSize)) {
			m_CurrentPath = builtPath;
			m_SelectedItems.clear();
			RefreshDirectoryCache();
		}
		ImGui::PopID();
	}
}

void ContentBrowserPanel::DrawFolderTree(FolderNode &node, std::string &selectedPath) {
	auto *vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (node.fullPath == selectedPath) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}
	if (node.children.empty() && node.loaded) {
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	ImGui::PushID(node.fullPath.c_str());

	std::string label = std::string(ICON_LC_FOLDER) + " " + node.name;
	bool open = ImGui::TreeNodeEx(label.c_str(), flags);

	if (ImGui::IsItemClicked()) {
		selectedPath = node.fullPath;
		m_CurrentPath = selectedPath;
		m_SelectedItems.clear();
		RefreshDirectoryCache();
	}

	if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
		m_ContextMenuTarget = node.fullPath;
		m_ShowFolderContextMenu = true;
	}

	if (open) {
		if (!node.loaded) {
			auto entries = vfs->ListDirectory(node.fullPath);
			for (const auto &entry : entries) {
				std::string childPath = node.fullPath + "/" + entry;
				if (vfs->IsDirectory(childPath)) {
					FolderNode child;
					child.fullPath = childPath;
					child.name = entry;
					node.children.push_back(child);
				}
			}
			node.loaded = true;
		}

		for (auto &child : node.children) {
			DrawFolderTree(child, selectedPath);
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

void ContentBrowserPanel::DrawContentGrid(f32 contentWidth) {
	auto it = m_DirectoryCache.find(m_CurrentPath);
	if (it == m_DirectoryCache.end() || it->second.needsRefresh) {
		RefreshDirectoryCache();
		it = m_DirectoryCache.find(m_CurrentPath);
	}

	if (it == m_DirectoryCache.end()) {
		ImGuiUtils::TextColored("Invalid directory", ImGuiUtils::Colors::Red());
		return;
	}

	auto &cache = it->second;

	FilterDirectoryCache();

	const auto &entries = cache.filteredEntries;

	if (entries.empty()) {
		const char *msg = strlen(static_cast<const char *>(m_SearchBuffer)) > 0 ? "No items match your search"
																				: "This folder is empty";
		ImVec2 textSize = ImGui::CalcTextSize(msg);
		ImGuiUtils::CenterCursor(textSize);
		ImGui::TextDisabled("%s", msg);
		return;
	}

	m_ItemPositions.clear();

	if (m_ViewMode == ViewMode::Grid) {
		DrawGridView(entries, contentWidth);
	} else {
		DrawListView(entries);
	}

	HandleSelectionRectangle();

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
		if (!ImGui::GetIO().KeyCtrl) {
			m_SelectedItems.clear();
			Aquila::Events::AssetSelectedEvent deselect("", "");
			m_App.OnEvent(deselect);
		}
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
		m_ContextMenuTarget = m_CurrentPath;
		m_ShowEmptySpaceContextMenu = true;
	}
}

void ContentBrowserPanel::DrawGridView(const std::vector<DirectoryEntry> &entries, f32 contentWidth) {
	const f32 thumbnailSize = m_ThumbnailSize;
	const f32 cellSize = thumbnailSize + 20.0f;
	const f32 padding = 16.0f;

	int columns = std::max(1, static_cast<int>((contentWidth - padding) / (cellSize + padding)));

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(padding, padding));
	ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.0f));

	int index = 0;
	for (const auto &entry : entries) {
		bool isSelected = m_SelectedItems.contains(entry.fullPath);

		if (index % columns != 0) {
			ImGui::SameLine();
		}

		ImGui::BeginGroup();
		ImGui::PushID(index);

		DrawGridItem(entry, isSelected, thumbnailSize);

		ImGui::PopID();
		ImGui::EndGroup();

		index++;
	}

	ImGui::PopStyleVar(2);
}

void ContentBrowserPanel::DrawGridItem(const DirectoryEntry &entry, bool isSelected, f32 thumbnailSize) {
	ImVec2 itemPos = ImGui::GetCursorScreenPos();
	const f32 cellSize = thumbnailSize + 20.0f;

	m_ItemPositions[entry.fullPath] = ImVec2(itemPos.x + cellSize * 0.5f, itemPos.y + (cellSize + 30.0f) * 0.5f);

	ImDrawList *drawList = ImGui::GetWindowDrawList();

	if (isSelected) {
		drawList->AddRectFilled(itemPos, ImVec2(itemPos.x + cellSize, itemPos.y + cellSize + 30.0f),
								IM_COL32(60, 120, 200, 100), 4.0f);
	}

	ImGui::InvisibleButton(entry.fullPath.c_str(), ImVec2(cellSize, cellSize + 30.0f));

	static bool itemWasDragged = false;

	bool hovered = ImGui::IsItemHovered();
	bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
	bool active = ImGui::IsItemActive();
	bool dragging = active && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 4.0f);
	bool released = ImGui::IsItemDeactivated();
	bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
	bool doubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered;

	if (dragging) {
		itemWasDragged = true;
	}

	if (!entry.isDirectory && ImGui::BeginDragDropSource()) {
		SetupDragDropPayload(entry.fullPath, entry.extension, entry.name);
		ImGui::EndDragDropSource();
	}

	if (clicked) {
		if (ImGui::GetIO().KeyCtrl) {
			if (isSelected) {
				m_SelectedItems.erase(entry.fullPath);
			} else {
				m_SelectedItems.insert(entry.fullPath);
			}
		} else if (ImGui::GetIO().KeyShift && !m_SelectedItems.empty()) {
			HandleRangeSelection(entry.fullPath);
		} else {
			m_SelectedItems.clear();
			m_SelectedItems.insert(entry.fullPath);
		}

		m_LastSelectedItem = entry.fullPath;
	}

	if (released) {
		if (!itemWasDragged) {
			if (!entry.isDirectory) {
				Aquila::Events::AssetSelectedEvent evt(entry.fullPath, entry.extension);
				m_App.OnEvent(evt);
			} else {
				Aquila::Events::AssetSelectedEvent deselect("", "");
				m_App.OnEvent(deselect);
			}
		}

		itemWasDragged = false;
	}

	if (doubleClicked) {
		if (entry.isDirectory) {
			m_CurrentPath = entry.fullPath;
			m_SelectedItems.clear();
			RefreshDirectoryCache();
		} else if (entry.extension == ".aqscene") {
			auto &assetManager = m_App.GetAssetManager();
			if (auto *scene = assetManager.LoadScene(entry.fullPath)) {
				assetManager.ActivateScene(scene);
				AQUILA_LOG_INFO("Scene loaded: {}", entry.fullPath);
			}
		} else if (entry.extension == ".slang") {
			ShowInExplorer(entry.fullPath);
		}
	}

	if (rightClicked) {
		if (!isSelected) {
			m_SelectedItems.clear();
			m_SelectedItems.insert(entry.fullPath);
		}
		m_ContextMenuTarget = entry.fullPath;
		m_ShowItemContextMenu = true;
	}

	if (hovered && !isSelected) {
		drawList->AddRect(itemPos, ImVec2(itemPos.x + cellSize, itemPos.y + cellSize + 30.0f),
						  IM_COL32(100, 140, 200, 150), 4.0f, 0, 2.0f);
	}

	if (isSelected) {
		drawList->AddRect(itemPos, ImVec2(itemPos.x + cellSize, itemPos.y + cellSize + 30.0f),
						  IM_COL32(80, 140, 255, 255), 4.0f, 0, 2.0f);
	}

	ImGui::SetCursorScreenPos(ImVec2(itemPos.x + 10.0f, itemPos.y + 10.0f));
	ImTextureID textureID = GetThumbnailForItem(entry.fullPath, entry.extension, entry.isDirectory);
	if (textureID != 0u) {
		ImGui::Image(textureID, ImVec2(thumbnailSize, thumbnailSize));
	}

	std::string displayName = entry.name;
	if (!entry.isDirectory) {
		if (size_t dot = displayName.find_last_of('.'); dot != std::string::npos) {
			displayName = displayName.substr(0, dot);
		}
	}

	if (displayName.length() > 15) {
		displayName = displayName.substr(0, 12) + "...";
	}

	ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
	ImGui::SetCursorScreenPos(ImVec2(itemPos.x + ((cellSize - textSize.x) * 0.5f), itemPos.y + cellSize + 5.0f));

	if (isSelected) {
		ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", displayName.c_str());
	} else {
		ImGui::Text("%s", displayName.c_str());
	}
}

void ContentBrowserPanel::DrawListView(const std::vector<DirectoryEntry> &entries) {
	ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
							ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable;

	if (ImGui::BeginTable("ContentList", 4, flags)) {
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (const auto &entry : entries) {
			bool isSelected = m_SelectedItems.find(entry.fullPath) != m_SelectedItems.end();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGuiSelectableFlags selectableFlags =
				ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;

			if (ImGui::Selectable(("##" + entry.name).c_str(), isSelected, selectableFlags)) {
				if (ImGui::GetIO().KeyCtrl) {
					if (isSelected) {
						m_SelectedItems.erase(entry.fullPath);
					} else {
						m_SelectedItems.insert(entry.fullPath);
					}
				} else {
					m_SelectedItems.clear();
					m_SelectedItems.insert(entry.fullPath);
				}

				if (!entry.isDirectory) {
					Aquila::Events::AssetSelectedEvent evt(entry.fullPath, entry.extension);
					m_App.OnEvent(evt);
				} else {
					Aquila::Events::AssetSelectedEvent deselect("", "");
					m_App.OnEvent(deselect);
				}
			}

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (entry.isDirectory) {
					m_CurrentPath = entry.fullPath;
					m_SelectedItems.clear();
					RefreshDirectoryCache();
				} else if (entry.extension == ".slang") {
					ShowInExplorer(entry.fullPath);
				}
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
				if (!isSelected) {
					m_SelectedItems.clear();
					m_SelectedItems.insert(entry.fullPath);
				}
				m_ContextMenuTarget = entry.fullPath;
				m_ShowItemContextMenu = true;
			}

			ImGui::SameLine();
			if (entry.isDirectory) {
				ImGui::Text(ICON_LC_FOLDER " %s", entry.name.c_str());
			} else if (entry.extension == ".slang") {
				ImGui::Text(ICON_LC_CODE " %s", entry.name.c_str());
			} else {
				ImGui::Text(ICON_LC_FILE " %s", entry.name.c_str());
			}

			if (!entry.isDirectory && ImGui::BeginDragDropSource()) {
				SetupDragDropPayload(entry.fullPath, entry.extension, entry.name);
				ImGui::EndDragDropSource();
			}

			ImGui::TableNextColumn();
			ImGui::Text("%s", entry.fileType.c_str());

			ImGui::TableNextColumn();
			ImGui::Text("%s", entry.isDirectory ? "--" : "N/A");

			ImGui::TableNextColumn();
			ImGui::Text("--");
		}

		ImGui::EndTable();
	}
}

void ContentBrowserPanel::DrawInfoPanel() {
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 4));

	if (m_SelectedItems.empty()) {
		ImGuiUtils::TextAligned("No items selected", ImGuiUtils::TextAlign::MiddleLeft, ImVec2(5, 5));
		ImGui::PopStyleVar();
		return;
	}

	if (m_SelectedItems.size() == 1) {
		std::string selectedPath = *m_SelectedItems.begin();
		auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();
		bool isDirectory = vfs->IsDirectory(selectedPath);

		size_t lastSlash = selectedPath.find_last_of('/');
		std::string name = (lastSlash != std::string::npos) ? selectedPath.substr(lastSlash + 1) : selectedPath;

		ImGuiUtils::HorizontalGroup(ImGuiUtils::TextAlign::MiddleLeft, ImVec2(5, 5), [&]() {
			ImGui::Text(ICON_LC_INFO " %s", name.c_str());

			ImGui::SameLine();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			ImGui::SameLine();

			ImGui::Text("Type: %s", GetFileTypeString(selectedPath).c_str());

			if (isDirectory) {
				auto entries = vfs->ListDirectory(selectedPath);
				ImGui::SameLine();
				ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
				ImGui::SameLine();
				ImGui::Text("Items: %zu", entries.size());
			}
		});
	} else {
		ImGuiUtils::TextAlignedFmt(ImGuiUtils::TextAlign::MiddleLeft, ImVec2(5, 5), ICON_LC_FILES " %zu items selected",
								   m_SelectedItems.size());
	}

	ImGui::PopStyleVar();
}

void ContentBrowserPanel::HandleContextMenus() {
	if (m_ShowItemContextMenu) {
		ImGui::OpenPopup("ItemContextMenu");
		m_ShowItemContextMenu = false;
	}
	if (m_ShowEmptySpaceContextMenu) {
		ImGui::OpenPopup("EmptySpaceContextMenu");
		m_ShowEmptySpaceContextMenu = false;
	}
	if (m_ShowFolderContextMenu) {
		ImGui::OpenPopup("FolderContextMenu");
		m_ShowFolderContextMenu = false;
	}

	if (ImGui::BeginPopup("ItemContextMenu")) {
		if (m_SelectedItems.size() == 1) {
			std::string ext = GetFileExtension(*m_SelectedItems.begin());
			if (ext == ".slang") {
				if (ImGui::MenuItem(ICON_LC_PALETTE " Create Material from Shader")) {
					m_ShowCreateMaterialPopup = true;

					strncpy(m_PendingShaderPath, m_ContextMenuTarget.c_str(), sizeof(m_PendingShaderPath) - 1);
					m_PendingShaderPath[sizeof(m_PendingShaderPath) - 1] = '\0';
					ImGui::CloseCurrentPopup();
				}
				ImGui::Separator();
			}
		}

		if (ImGui::MenuItem(ICON_LC_COPY " Copy")) {
			m_ClipboardPaths = m_SelectedItems;
			m_ClipboardIsCut = false;
			AQUILA_LOG_INFO("Copied {} items to clipboard", m_ClipboardPaths.size());
		}

		if (ImGui::MenuItem(ICON_LC_SCISSORS " Cut")) {
			m_ClipboardPaths = m_SelectedItems;
			m_ClipboardIsCut = true;
			AQUILA_LOG_INFO("Cut {} items to clipboard", m_ClipboardPaths.size());
		}

		if (ImGui::MenuItem(ICON_LC_TRASH_2 " Delete")) {
			m_ShowDeletePopup = true;
			ImGui::CloseCurrentPopup();
		}

		if (m_SelectedItems.size() == 1) {
			if (ImGui::MenuItem(ICON_LC_PEN " Rename")) {
				m_ShowRenamePopup = true;

				size_t lastSlash = m_ContextMenuTarget.find_last_of('/');
				std::string currentName =
					(lastSlash != std::string::npos) ? m_ContextMenuTarget.substr(lastSlash + 1) : m_ContextMenuTarget;

				strncpy(m_RenameBuffer, currentName.c_str(), sizeof(m_RenameBuffer) - 1);
				m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';

				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_FILE " Show in Explorer")) {
			ShowInExplorer(m_ContextMenuTarget);
		}

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("EmptySpaceContextMenu")) {
		if (ImGui::MenuItem(ICON_LC_FOLDER_PLUS " New Folder")) {
			m_ShowCreateFolderPopup = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::Separator();

		if (ImGui::BeginMenu(ICON_LC_FILE_PLUS " Create")) {
			if (ImGui::MenuItem("Material")) {
				m_ShowCreateMaterialPopup = true;
				memset(m_PendingShaderPath, 0, sizeof(m_PendingShaderPath));
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Slang Shader")) {
				m_ShowCreateSlangPopup = true;
				ImGui::CloseCurrentPopup();
			}
			if (ImGui::MenuItem("Script")) {
			}
			ImGui::EndMenu();
		}

		ImGui::Separator();

		bool canPaste = !m_ClipboardPaths.empty();

		{
			ImGuiUtils::ScopedDisable disable(!canPaste);
			if (ImGui::MenuItem(ICON_LC_CLIPBOARD " Paste")) {
				PasteFromClipboard(m_CurrentPath);
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem(ICON_LC_REFRESH_CW " Refresh")) {
			m_ImagePreviews.clear();
			auto &cache = m_DirectoryCache[m_CurrentPath];
			cache.needsRefresh = true;
			RefreshDirectoryCache();
		}

		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("FolderContextMenu")) {
		if (ImGui::MenuItem(ICON_LC_FOLDER_PLUS " New Subfolder")) {
			m_ShowCreateFolderPopup = true;
			m_CurrentPath = m_ContextMenuTarget;
			RefreshDirectoryCache();
			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem(ICON_LC_PEN " Rename")) {
			m_ShowRenamePopup = true;

			size_t lastSlash = m_ContextMenuTarget.find_last_of('/');
			std::string currentName =
				(lastSlash != std::string::npos) ? m_ContextMenuTarget.substr(lastSlash + 1) : m_ContextMenuTarget;

			strncpy(m_RenameBuffer, currentName.c_str(), sizeof(m_RenameBuffer) - 1);
			m_RenameBuffer[sizeof(m_RenameBuffer) - 1] = '\0';

			ImGui::CloseCurrentPopup();
		}

		if (ImGui::MenuItem(ICON_LC_TRASH_2 " Delete")) {
			m_SelectedItems.clear();
			m_SelectedItems.insert(m_ContextMenuTarget);
			m_ShowDeletePopup = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void ContentBrowserPanel::HandlePopups() {
	if (m_ShowDeletePopup) {
		ImGui::OpenPopup("Delete Items");
		m_ShowDeletePopup = false;
	}

	if (ImGui::BeginPopupModal("Delete Items", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		if (m_SelectedItems.size() == 1) {
			ImGui::Text("Are you sure you want to delete:");
			ImGuiUtils::TextColored(m_ContextMenuTarget.c_str(), ImGuiUtils::Colors::Red());
		} else {
			ImGui::Text("Are you sure you want to delete %zu items?", m_SelectedItems.size());
		}
		ImGui::Separator();

		if (ImGui::Button("Delete", ImVec2(120, 0))) {
			auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

			for (const auto &path : m_SelectedItems) {
				if (vfs->IsDirectory(path)) {
					if (vfs->DeleteDirectory(path)) {
						AQUILA_LOG_INFO("Deleted directory: {}", path);
					} else {
						AQUILA_LOG_ERROR("Failed to delete directory: {}", path);
					}
				} else {
					if (vfs->DeleteFile_aq(path)) {
						AQUILA_LOG_INFO("Deleted file: {}", path);
					} else {
						AQUILA_LOG_ERROR("Failed to delete file: {}", path);
					}
				}
			}

			m_SelectedItems.clear();

			auto &cache = m_DirectoryCache[m_CurrentPath];
			cache.needsRefresh = true;
			RefreshDirectoryCache();

			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (m_ShowRenamePopup) {
		ImGui::OpenPopup("Rename Item");
		m_ShowRenamePopup = false;
		m_RenameFocusRequested = true;
	}

	if (ImGui::BeginPopupModal("Rename Item", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("New name:");

		if (m_RenameFocusRequested) {
			ImGui::SetKeyboardFocusHere();
			m_RenameFocusRequested = false;
		}

		bool enterPressed =
			ImGui::InputText("##rename", m_RenameBuffer, sizeof(m_RenameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

		if (ImGui::Button("Rename", ImVec2(120, 0)) || enterPressed) {
			if (strlen(m_RenameBuffer) > 0) {
				size_t lastSlash = m_ContextMenuTarget.find_last_of('/');
				std::string parentPath =
					(lastSlash != std::string::npos) ? m_ContextMenuTarget.substr(0, lastSlash + 1) : "assets://";

				std::string newPath = parentPath + std::string(m_RenameBuffer);

				auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

				if (!vfs->Exists(newPath)) {
					if (vfs->RenameFile(m_ContextMenuTarget, newPath)) {
						AQUILA_LOG_INFO("Renamed: {} -> {}", m_ContextMenuTarget, newPath);

						if (m_SelectedItems.contains(m_ContextMenuTarget)) {
							m_SelectedItems.erase(m_ContextMenuTarget);
							m_SelectedItems.insert(newPath);
						}

						if (m_CurrentPath == m_ContextMenuTarget) {
							m_CurrentPath = newPath;
						}

						auto &cache = m_DirectoryCache[m_CurrentPath];
						cache.needsRefresh = true;
						RefreshDirectoryCache();
					} else {
						AQUILA_LOG_ERROR("Failed to rename: {} -> {}", m_ContextMenuTarget, newPath);
					}
				} else {
					AQUILA_LOG_WARNING("Rename failed: Path already exists");
				}
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (m_ShowCreateFolderPopup) {
		ImGui::OpenPopup("Create Folder");
		m_ShowCreateFolderPopup = false;
		memset(m_NewFolderBuffer, 0, sizeof(m_NewFolderBuffer));
		m_CreateFolderFocusRequested = true;
	}

	if (ImGui::BeginPopupModal("Create Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Folder name:");

		if (m_CreateFolderFocusRequested) {
			ImGui::SetKeyboardFocusHere();
			m_CreateFolderFocusRequested = false;
		}

		bool enterPressed = ImGui::InputText("##newfolder", m_NewFolderBuffer, sizeof(m_NewFolderBuffer),
											 ImGuiInputTextFlags_EnterReturnsTrue);

		if (ImGui::Button("Create", ImVec2(120, 0)) || enterPressed) {
			if (strlen(m_NewFolderBuffer) > 0) {
				std::string newPath = m_CurrentPath;
				if (newPath.back() != '/') {
					newPath += "/";
				}
				newPath += m_NewFolderBuffer;

				auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();
				if (vfs->CreateDir(newPath)) {
					AQUILA_LOG_INFO("Created folder: {}", newPath);

					auto &cache = m_DirectoryCache[m_CurrentPath];
					cache.needsRefresh = true;
					RefreshDirectoryCache();
				} else {
					AQUILA_LOG_ERROR("Failed to create folder: {}", newPath);
				}
			}
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (m_ShowCreateSlangPopup) {
		ImGui::OpenPopup("Create Slang Shader");
		m_ShowCreateSlangPopup = false;
		memset(m_NewSlangNameBuffer, 0, sizeof(m_NewSlangNameBuffer));
		m_CreateSlangFocusRequested = true;
	}

	if (ImGui::BeginPopupModal("Create Slang Shader", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Shader name (without extension):");

		if (m_CreateSlangFocusRequested) {
			ImGui::SetKeyboardFocusHere();
			m_CreateSlangFocusRequested = false;
		}

		bool enterPressed = ImGui::InputText("##slangname", m_NewSlangNameBuffer, sizeof(m_NewSlangNameBuffer),
											 ImGuiInputTextFlags_EnterReturnsTrue);

		ImGui::Spacing();
		ImGui::Text("Template:");

		static int s_SlangTemplate = 0;
		ImGui::RadioButton("PBR (Vertex + Fragment)", &s_SlangTemplate, 0);
		ImGui::RadioButton("Unlit (Vertex + Fragment)", &s_SlangTemplate, 1);
		ImGui::RadioButton("Compute", &s_SlangTemplate, 2);
		ImGui::RadioButton("Empty", &s_SlangTemplate, 3);

		ImGui::Spacing();

		bool canCreate = strlen(m_NewSlangNameBuffer) > 0;
		{
			ImGuiUtils::ScopedDisable disable(!canCreate);
			if (ImGui::Button("Create", ImVec2(120, 0)) || (enterPressed && canCreate)) {
				std::string savePath = m_CurrentPath;
				if (savePath.back() != '/') {
					savePath += "/";
				}
				savePath += std::string(m_NewSlangNameBuffer) + ".slang";

				auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

				if (vfs->Exists(savePath)) {
					AQUILA_LOG_WARNING("Shader file already exists: {}", savePath);
				} else {
					std::string templateSource = GetSlangTemplate(s_SlangTemplate, m_NewSlangNameBuffer);
					if (vfs->WriteTextFile(savePath, templateSource)) {
						AQUILA_LOG_INFO("Created Slang shader: {}", savePath);

						auto &cache = m_DirectoryCache[m_CurrentPath];
						cache.needsRefresh = true;
						RefreshDirectoryCache();
					} else {
						AQUILA_LOG_ERROR("Failed to write Slang shader: {}", savePath);
					}
				}

				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	if (m_ShowCreateMaterialPopup) {
		ImGui::OpenPopup("CreateMaterial");
		m_ShowCreateMaterialPopup = false;
		memset(m_NewMaterialNameBuffer, 0, sizeof(m_NewMaterialNameBuffer));
		m_CreateMaterialFocusRequested = true;
	}

	static int s_MaterialCreationMode = 0;
	static int s_TemplateIdx = 0;

	if (ImGui::BeginPopupModal("CreateMaterial", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Create New Material");
		ImGui::Separator();

		if (m_CreateMaterialFocusRequested) {
			ImGui::SetKeyboardFocusHere();
			m_CreateMaterialFocusRequested = false;
		}

		ImGui::InputText("Name##matname", m_NewMaterialNameBuffer, sizeof(m_NewMaterialNameBuffer));

		ImGui::Spacing();

		ImGui::RadioButton("Template", &s_MaterialCreationMode, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Custom Shader (.slang)", &s_MaterialCreationMode, 1);

		ImGui::Spacing();

		if (s_MaterialCreationMode == 0) {
			const char *templates[] = { "PBR", "Unlit", "Transparent" };
			ImGui::Combo("Template##tmpl", &s_TemplateIdx, templates, 3);
		} else {
			ImGui::Text("Shader path:");
			ImGui::InputText("##ShaderPath", m_PendingShaderPath, sizeof(m_PendingShaderPath));

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("SHADER_ASSET")) {
					strncpy(m_PendingShaderPath, static_cast<const char *>(payload->Data),
							sizeof(m_PendingShaderPath) - 1);
					m_PendingShaderPath[sizeof(m_PendingShaderPath) - 1] = '\0';
				}
				ImGui::EndDragDropTarget();
			}

			if (strlen(m_PendingShaderPath) > 0) {
				ImGui::TextDisabled(ICON_LC_INFO " Shader will be compiled on material creation");
			}
		}

		ImGui::Spacing();

		bool canCreate =
			strlen(m_NewMaterialNameBuffer) > 0 && (s_MaterialCreationMode == 0 || strlen(m_PendingShaderPath) > 0);

		{
			ImGuiUtils::ScopedDisable disable(!canCreate);
			if (ImGui::Button("Create", ImVec2(120, 0))) {
				auto &matLibrary = m_App.GetMaterialSystem().GetLibrary();

				std::string matName = m_NewMaterialNameBuffer;
				std::string savePath = m_CurrentPath;
				if (savePath.back() != '/') {
					savePath += "/";
				}
				savePath += matName + ".aqmat";

				auto *vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();
				if (vfs->Exists(savePath)) {
					int counter = 1;
					std::string candidate = savePath;
					while (vfs->Exists(candidate) && counter < 1000) {
						candidate = m_CurrentPath;
						if (candidate.back() != '/') {
							candidate += "/";
						}
						candidate += matName + "_" + std::to_string(counter) + ".aqmat";
						counter++;
					}
					savePath = candidate;
				}

				Ref<Aquila::Graphics::Material::Material> mat = nullptr;

				if (s_MaterialCreationMode == 0) {
					const char *templates[] = { "PBR", "Unlit", "Transparent" };
					mat = matLibrary.CreateMaterial(matName, templates[s_TemplateIdx]);
					AQUILA_LOG_INFO("Creating material '{}' from template '{}'", matName, templates[s_TemplateIdx]);
				} else {
					std::string shaderPath = m_PendingShaderPath;
					std::string errorLog;

					auto shader =
						CreateRef<Aquila::Graphics::Shader::ShaderProgram>(m_App.GetDevice(), matName + "_shader");
					shader->SetTargetFormats(Aquila::Graphics::Helpers::PipelineRenderingFormats::GBuffer());

					if (!shader->AddStageFromSlang(shaderPath, errorLog)) {
						AQUILA_LOG_ERROR("Failed to compile shader '{}': {}", shaderPath, errorLog);
					} else if (!shader->Reflect()) {
						AQUILA_LOG_ERROR("Failed to reflect shader '{}'", shaderPath);
					} else {
						mat = matLibrary.CreateMaterialFromShader(matName, shader);
						if (mat) {
							mat->SetShaderAsset(shaderPath);
							AQUILA_LOG_INFO("Created material '{}' from shader '{}'", matName, shaderPath);
						} else {
							AQUILA_LOG_ERROR("CreateMaterialFromShader returned null for '{}'", matName);
						}
					}
				}

				if (mat) {
					auto &assetManager = m_App.GetAssetManager();
					if (assetManager.SaveMaterialAsset(mat, savePath)) {
						AQUILA_LOG_INFO("Material saved to: {}", savePath);

						auto &cache = m_DirectoryCache[m_CurrentPath];
						cache.needsRefresh = true;
						RefreshDirectoryCache();
					} else {
						AQUILA_LOG_ERROR("Failed to save material to: {}", savePath);
					}
				}

				memset(m_NewMaterialNameBuffer, 0, sizeof(m_NewMaterialNameBuffer));
				memset(m_PendingShaderPath, 0, sizeof(m_PendingShaderPath));
				s_MaterialCreationMode = 0;
				s_TemplateIdx = 0;

				ImGui::CloseCurrentPopup();
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			memset(m_NewMaterialNameBuffer, 0, sizeof(m_NewMaterialNameBuffer));
			memset(m_PendingShaderPath, 0, sizeof(m_PendingShaderPath));
			s_MaterialCreationMode = 0;
			s_TemplateIdx = 0;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void ContentBrowserPanel::HandleSelectionRectangle() {
	ImGuiIO &io = ImGui::GetIO();

	if (m_ViewMode != ViewMode::Grid) {
		return;
	}

	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() &&
		!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		if (!io.KeyCtrl && !io.KeyShift) {
			m_IsSelectingWithRect = true;
			m_SelectionRectStart = ImGui::GetMousePos();
			m_SelectionRectEnd = m_SelectionRectStart;
		}
	}

	if (m_IsSelectingWithRect && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
		m_SelectionRectEnd = ImGui::GetMousePos();

		ImDrawList *drawList = ImGui::GetWindowDrawList();

		ImVec2 rectMin = ImVec2(std::min(m_SelectionRectStart.x, m_SelectionRectEnd.x),
								std::min(m_SelectionRectStart.y, m_SelectionRectEnd.y));

		ImVec2 rectMax = ImVec2(std::max(m_SelectionRectStart.x, m_SelectionRectEnd.x),
								std::max(m_SelectionRectStart.y, m_SelectionRectEnd.y));

		drawList->AddRectFilled(rectMin, rectMax, IM_COL32(80, 140, 255, 30));
		drawList->AddRect(rectMin, rectMax, IM_COL32(80, 140, 255, 200), 0.0f, 0, 2.0f);

		if (!io.KeyCtrl) {
			m_SelectedItems.clear();
		}

		for (const auto &[path, pos] : m_ItemPositions) {
			bool isInRect = (pos.x >= rectMin.x && pos.x <= rectMax.x && pos.y >= rectMin.y && pos.y <= rectMax.y);

			if (isInRect) {
				m_SelectedItems.insert(path);
			}
		}
	}

	if (m_IsSelectingWithRect && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		m_IsSelectingWithRect = false;
	}
}

void ContentBrowserPanel::HandleRangeSelection(const std::string &endPath) {
	m_SelectedItems.clear();
	m_SelectedItems.insert(endPath);
}

ImTextureID ContentBrowserPanel::GetThumbnailForItem(const std::string &fullPath, const std::string &extension,
													 bool isDirectory) {
	auto &assetManager = m_App.GetAssetManager();

	if (isDirectory) {
		return reinterpret_cast<ImTextureID>(m_Textures[0]->GetDescriptorSet());
	}

	if (extension == ".aqscene") {
		return reinterpret_cast<ImTextureID>(m_Textures[1]->GetDescriptorSet());
	}

	if (extension == ".png" || extension == ".jpg" || extension == ".tga" || extension == ".hdr") {
		if (auto previewTexture = assetManager.TryGetTexture(fullPath)) {
			return reinterpret_cast<ImTextureID>(previewTexture->GetDescriptorSet());
		}
	}

	return reinterpret_cast<ImTextureID>(m_Textures[2]->GetDescriptorSet());
}

void ContentBrowserPanel::SetupDragDropPayload(const std::string &fullPath, const std::string &extension,
											   const std::string &entryName) {
	static char dragDropPathBuffer[512];
	strncpy(dragDropPathBuffer, fullPath.c_str(), sizeof(dragDropPathBuffer) - 1);
	dragDropPathBuffer[sizeof(dragDropPathBuffer) - 1] = '\0';

	if (extension == ".aqscene") {
		ImGui::SetDragDropPayload("SCENE_PATH", dragDropPathBuffer, strlen(dragDropPathBuffer) + 1);
		ImGui::Text(ICON_LC_FILE " %s", entryName.c_str());
	} else if (extension == ".hdr") {
		ImGui::SetDragDropPayload("HDR_ASSET", dragDropPathBuffer, strlen(dragDropPathBuffer) + 1);
		ImGui::Text(ICON_LC_SUN " %s", entryName.c_str());
	} else if (extension == ".gltf" || extension == ".glb" || extension == ".obj" || extension == ".fbx") {
		ImGui::SetDragDropPayload("MESH_ASSET", dragDropPathBuffer, strlen(dragDropPathBuffer) + 1);
		ImGui::Text(ICON_LC_BOX " %s", entryName.c_str());
	} else if (extension == ".png" || extension == ".jpg" || extension == ".tga") {
		ImGui::SetDragDropPayload("TEXTURE_ASSET", dragDropPathBuffer, strlen(dragDropPathBuffer) + 1);
		ImGui::Text(ICON_LC_IMAGE " %s", entryName.c_str());
	} else if (extension == ".aqmat") {
		ImGui::SetDragDropPayload("MATERIAL_ASSET", dragDropPathBuffer, strlen(dragDropPathBuffer) + 1);
		ImGui::Text(ICON_LC_PALETTE " %s", entryName.c_str());
	} else if (extension == ".slang") {
		ImGui::SetDragDropPayload("SHADER_ASSET", dragDropPathBuffer, strlen(dragDropPathBuffer) + 1);
		ImGui::Text(ICON_LC_CODE " %s", entryName.c_str());
	} else {
		ImGui::Text(ICON_LC_FILE " %s", entryName.c_str());
	}
}

std::string ContentBrowserPanel::GetFileTypeString(const std::string &path) {
	auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

	if (vfs->IsDirectory(path)) {
		return "Folder";
	}

	std::string extension = GetFileExtension(path);

	if (extension == ".aqscene") {
		return "Scene";
	}
	if (extension == ".hdr") {
		return "HDR Texture";
	}
	if (extension == ".png") {
		return "PNG Image";
	}
	if (extension == ".jpg") {
		return "JPEG Image";
	}
	if (extension == ".tga") {
		return "TGA Image";
	}
	if (extension == ".gltf" || extension == ".glb") {
		return "glTF Model";
	}
	if (extension == ".obj") {
		return "OBJ Model";
	}
	if (extension == ".fbx") {
		return "FBX Model";
	}
	if (extension == ".aqmat") {
		return "Material";
	}
	if (extension == ".slang") {
		return "Shader";
	}

	return "File";
}

std::string ContentBrowserPanel::GetSlangTemplate(int templateIndex, const std::string &shaderName) {
	switch (templateIndex) {
	case 0:
		return R"(


[[vk::binding(0, 0)]]
cbuffer CameraData {
	column_major f324x4 cameraView;
	column_major f324x4 cameraProjection;
	column_major f324x4 cameraViewProjection;
	column_major f324x4 cameraInverseView;
	column_major f324x4 cameraInverseProjection;
	f323 cameraPosition;
	f32 nearPlane;
	f32 farPlane;
};


[[vk::binding(0, 1)]]
Sampler2D albedoMap;
[[vk::binding(1, 1)]]
Sampler2D normalMap;
[[vk::binding(2, 1)]]
Sampler2D aoMap;
[[vk::binding(3, 1)]]
Sampler2D emissiveMap;

[[vk::binding(4, 1)]]
cbuffer MaterialUBO {
	f324 albedoColor;
	f324 emissiveColor;
	f32 metallic;
	f32 roughness;
};

struct PushConstants {
	column_major f324x4 modelMatrix;
	column_major f324x4 normalMatrix;
};

[[vk::push_constant]]
PushConstants push;

struct VSInput {
	[[vk::location(0)]]
	f323 position : POSITION;
	[[vk::location(1)]]
	f323 color : COLOR;
	[[vk::location(2)]]
	f323 normal : NORMAL;
	[[vk::location(3)]]
	f324 tangent : TANGENT;
	[[vk::location(4)]]
	f322 uv : TEXCOORD0;
};

struct VSOutput {
	f324 sv_position : SV_Position;
	[[vk::location(0)]]
	f323 fragColor : TEXCOORD0;
	[[vk::location(1)]]
	f323 fragPosWorld : TEXCOORD1;
	[[vk::location(2)]]
	f323 fragNormal : TEXCOORD2;
	[[vk::location(3)]]
	f323 fragTangent : TEXCOORD3;
	[[vk::location(4)]]
	f323 fragBitangent : TEXCOORD4;
	[[vk::location(5)]]
	f322 fragUV : TEXCOORD5;
};

struct FSOutput {
	[[vk::location(0)]]
	f324 outPosition : SV_Target0;
	[[vk::location(1)]]
	f324 outNormal : SV_Target1;
	[[vk::location(2)]]
	f324 outAlbedo : SV_Target2;
};

[shader("vertex")]
VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;

	f324 posWorld = mul(push.modelMatrix, f324(IN.position, 1.0));
	f323x3 normalMat = (f323x3)push.normalMatrix;

	f323 N = normalize(mul(normalMat, IN.normal));
	OUT.fragNormal = N;
	OUT.fragPosWorld = posWorld.xyz;
	OUT.fragColor = IN.color;
	OUT.fragUV = IN.uv;

	if (length(IN.tangent.xyz) > 0.01) {
		f323 T = normalize(mul(normalMat, IN.tangent.xyz));
		T = normalize(T - dot(T, N) * N);
		f323 B = cross(N, T) * IN.tangent.w;
		OUT.fragTangent = T;
		OUT.fragBitangent = B;
	} else {
		OUT.fragTangent = f323(1, 0, 0);
		OUT.fragBitangent = f323(0, 1, 0);
	}

	OUT.sv_position = mul(cameraProjection, mul(cameraView, posWorld));

	return OUT;
}

[shader("fragment")]
FSOutput fragmentMain(VSOutput IN) {
	f323x3 TBN = f323x3(normalize(IN.fragTangent), normalize(IN.fragBitangent), normalize(IN.fragNormal));

	f323 sampledAlbedo = pow(albedoMap.Sample(IN.fragUV).rgb, f323(2.2, 2.2, 2.2));
	f32 sampledAO = aoMap.Sample(IN.fragUV).r;
	f323 sampledNormal = normalMap.Sample(IN.fragUV).rgb;

	f323 localNormal = sampledNormal * 2.0 - 1.0;
	f323 worldNormal = normalize(mul(localNormal, TBN));

	f323 albedo = sampledAlbedo * albedoColor.rgb;

	FSOutput OUT;
	OUT.outPosition = f324(IN.fragPosWorld, metallic);
	OUT.outNormal = f324(worldNormal * 0.5 + 0.5, roughness);
	OUT.outAlbedo = f324(albedo, sampledAO);

	return OUT;
}
)";

	case 1:
		return R"(

[[vk::binding(0, 0)]]
cbuffer CameraData {
	column_major f324x4 cameraView;
	column_major f324x4 cameraProjection;
	column_major f324x4 cameraViewProjection;
	column_major f324x4 cameraInverseView;
	column_major f324x4 cameraInverseProjection;
	f323 cameraPosition;
	f32 nearPlane;
	f32 farPlane;
};

[[vk::binding(0, 1)]]
Sampler2D mainTexture;

struct PushConstants {
	column_major f324x4 modelMatrix;
	column_major f324x4 normalMatrix;
};

[[vk::push_constant]]
PushConstants push;

struct VSInput {
	[[vk::location(0)]]
	f323 position : POSITION;
	[[vk::location(1)]]
	f323 color : COLOR;
	[[vk::location(2)]]
	f323 normal : NORMAL;
	[[vk::location(3)]]
	f324 tangent : TANGENT;
	[[vk::location(4)]]
	f322 uv : TEXCOORD0;
};

struct VSOutput {
	f324 sv_position : SV_Position;
	[[vk::location(0)]]
	f322 fragUV : TEXCOORD0;
	[[vk::location(1)]]
	f324 fragColor : TEXCOORD1;
};

[shader("vertex")]
VSOutput vertexMain(VSInput IN) {
	VSOutput OUT;
	OUT.sv_position = mul(cameraProjection, mul(cameraView, mul(push.modelMatrix, f324(IN.position, 1.0))));
	OUT.fragUV = IN.uv;
	OUT.fragColor = f324(IN.color, 1.0);
	return OUT;
}

[shader("fragment")]
f324 fragmentMain(VSOutput IN) : SV_Target {
	return mainTexture.Sample(IN.fragUV) * IN.fragColor;
}
)";

	case 2:
		return R"(

[[vk::binding(0, 0)]] RWTexture2D<f324> outputImage;

[shader("compute")]
[numthreads(8, 8, 1)]
void computeMain(uint3 id : SV_DispatchThreadID) {
	uint width, height;
	outputImage.GetDimensions(width, height);

	if (id.x >= width || id.y >= height) return;

	f322 uv = f322(id.xy) / f322(width, height);
	outputImage[id.xy] = f324(uv, 0.0, 1.0);
}
)";

	default:
		return R"(

[[vk::binding(0, 0)]]
cbuffer CameraData {
	column_major f324x4 cameraView;
	column_major f324x4 cameraProjection;
	column_major f324x4 cameraViewProjection;
	column_major f324x4 cameraInverseView;
	column_major f324x4 cameraInverseProjection;
	f323 cameraPosition;
	f32 nearPlane;
	f32 farPlane;
};

struct PushConstants {
	column_major f324x4 modelMatrix;
	column_major f324x4 normalMatrix;
};

[[vk::push_constant]]
PushConstants push;

[shader("vertex")]
f324 vertexMain(f323 position : POSITION) : SV_Position {
	return f324(position, 1.0);
}

[shader("fragment")]
f324 fragmentMain() : SV_Target {
	return f324(1.0, 0.0, 1.0, 1.0);
}
)";
	}
}

void ContentBrowserPanel::PreloadTexturesRecursive(const std::string &basePath) {
	auto *vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();
	if (!vfs->IsDirectory(basePath)) {
		return;
	}

	auto entries = vfs->ListDirectory(basePath);
	auto &assetManager = m_App.GetAssetManager();

	for (const auto &entryName : entries) {
		std::string fullPath = basePath;
		if (fullPath.back() != '/') {
			fullPath += "/";
		}
		fullPath += entryName;

		if (vfs->IsDirectory(fullPath)) {
			PreloadTexturesRecursive(fullPath);
		} else {
			std::string extension = GetFileExtension(entryName);

			if (extension == ".hdr") {
				assetManager.LoadHDRTextureAsync(fullPath, Aquila::Core::JobPriority::Low);
			} else if (extension == ".png" || extension == ".jpg" || extension == ".tga") {
				assetManager.LoadTextureAsync(fullPath, VK_FORMAT_R8G8B8A8_UNORM, Aquila::Core::JobPriority::Low);
			}
		}
	}

	AQUILA_LOG_INFO("Queued texture preloads from: {}", basePath);
}

void ContentBrowserPanel::LoadContentBrowserTextures() {
	AQUILA_LOG_INFO("Content browser textures loaded");
}

std::string ContentBrowserPanel::GetFileExtension(const std::string &filename) {
	size_t dotPos = filename.find_last_of('.');
	if (dotPos != std::string::npos && dotPos != filename.length() - 1) {
		return filename.substr(dotPos);
	}
	return "";
}

std::string ContentBrowserPanel::GetParentPath(const std::string &path) {
	size_t lastSlash = path.find_last_of('/');
	if (lastSlash != std::string::npos && lastSlash > 0) {
		if (lastSlash >= 2 && path[lastSlash - 1] == '/' && path[lastSlash - 2] == ':') {
			return path.substr(0, lastSlash + 1);
		}
		return path.substr(0, lastSlash);
	}
	return "assets://";
}

void ContentBrowserPanel::PasteFromClipboard(const std::string &destinationPath) {
	if (m_ClipboardPaths.empty()) {
		AQUILA_LOG_WARNING("Clipboard is empty");
		return;
	}

	auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

	for (const auto &clipboardPath : m_ClipboardPaths) {
		if (!vfs->Exists(clipboardPath)) {
			AQUILA_LOG_ERROR("Clipboard source no longer exists: {}", clipboardPath);
			continue;
		}

		size_t lastSlash = clipboardPath.find_last_of('/');
		std::string itemName = (lastSlash != std::string::npos) ? clipboardPath.substr(lastSlash + 1) : clipboardPath;

		std::string destPath = destinationPath;
		if (destPath.back() != '/') {
			destPath += "/";
		}
		destPath += itemName;

		if (vfs->Exists(destPath)) {
			std::string baseName = itemName;
			std::string extension;

			size_t dotPos = itemName.find_last_of('.');
			if (dotPos != std::string::npos && !vfs->IsDirectory(clipboardPath)) {
				baseName = itemName.substr(0, dotPos);
				extension = itemName.substr(dotPos);
			}

			int counter = 1;
			do {
				destPath = destinationPath;
				if (destPath.back() != '/') {
					destPath += "/";
				}
				destPath += baseName + " (" + std::to_string(counter) + ")" + extension;
				counter++;
			} while (vfs->Exists(destPath) && counter < 1000);

			if (counter >= 1000) {
				AQUILA_LOG_ERROR("Too many duplicates, aborting paste");
				continue;
			}
		}

		bool success = false;

		if (m_ClipboardIsCut) {
			success = vfs->RenameFile(clipboardPath, destPath);
			if (success) {
				AQUILA_LOG_INFO("Moved: {} -> {}", clipboardPath, destPath);

				if (m_SelectedItems.count(clipboardPath)) {
					m_SelectedItems.erase(clipboardPath);
					m_SelectedItems.insert(destPath);
				}
			} else {
				AQUILA_LOG_ERROR("Failed to move: {} -> {}", clipboardPath, destPath);
			}
		} else {
			if (vfs->IsDirectory(clipboardPath)) {
				success = CopyDirectoryRecursive(clipboardPath, destPath);
			} else {
				success = vfs->CopyFile(clipboardPath, destPath);
			}

			if (success) {
				AQUILA_LOG_INFO("Copied: {} -> {}", clipboardPath, destPath);
			} else {
				AQUILA_LOG_ERROR("Failed to copy: {} -> {}", clipboardPath, destPath);
			}
		}
	}

	if (m_ClipboardIsCut) {
		m_ClipboardPaths.clear();
		m_ClipboardIsCut = false;
	}

	auto &cache = m_DirectoryCache[m_CurrentPath];
	cache.needsRefresh = true;
	RefreshDirectoryCache();
}

bool ContentBrowserPanel::CopyDirectoryRecursive(const std::string &srcPath, const std::string &dstPath) {
	auto vfs = Aquila::Platform::Filesystem::VirtualFileSystem::Get();

	if (!vfs->CreateDir(dstPath)) {
		AQUILA_LOG_ERROR("Failed to create directory: {}", dstPath);
		return false;
	}

	auto entries = vfs->ListDirectory(srcPath);

	for (const auto &entryName : entries) {
		std::string srcFullPath = srcPath;
		if (srcFullPath.back() != '/') {
			srcFullPath += "/";
		}
		srcFullPath += entryName;

		std::string dstFullPath = dstPath;
		if (dstFullPath.back() != '/') {
			dstFullPath += "/";
		}
		dstFullPath += entryName;

		if (vfs->IsDirectory(srcFullPath)) {
			if (!CopyDirectoryRecursive(srcFullPath, dstFullPath)) {
				return false;
			}
		} else {
			if (!vfs->CopyFile(srcFullPath, dstFullPath)) {
				AQUILA_LOG_ERROR("Failed to copy file: {} -> {}", srcFullPath, dstFullPath);
				return false;
			}
		}
	}

	return true;
}

void ContentBrowserPanel::ShowInExplorer(const std::string &virtualPath) {
	AQUILA_LOG_INFO("Show in explorer requested for: {}", virtualPath);
}

void ContentBrowserPanel::OnEvent(Aquila::Events::Event &event) {
	Aquila::Events::EventDispatcher dispatcher(event);

	dispatcher.Dispatch<Aquila::Events::SceneLoadedEvent>([](Aquila::Events::SceneLoadedEvent &e) {
		AQUILA_LOG_INFO("Scene loaded: {}", e.GetScenePath());
		return false;
	});
}

} // namespace Editor
