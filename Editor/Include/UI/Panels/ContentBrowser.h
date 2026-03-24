#pragma once

#include "Aquila/Core/Layer.h"
#include "Aquila/Graphics/Resources/Texture2D.h"
#include "Aquila/Foundation/SharedConstants.h"
#include "UI/ImGuiUtils.h"
#include <unordered_map>

namespace Aquila::Core {
class Application;
}

namespace Editor {

class ContentBrowserPanel : public Aquila::Core::Layer {
	struct FolderNode {
		std::string fullPath;
		std::string name;
		bool loaded = false;
		std::vector<FolderNode> children;
	};

	struct DirectoryEntry {
		std::string name;
		std::string fullPath;
		std::string extension;
		std::string fileType;
		bool isDirectory;
	};

	struct DirectoryCache {
		std::vector<DirectoryEntry> entries;
		std::vector<DirectoryEntry> filteredEntries;
		std::string lastSearchQuery;
		bool needsRefresh = true;
	};

  public:
	explicit ContentBrowserPanel(Aquila::Core::Application &app);
	~ContentBrowserPanel() override = default;

	void OnAttach() override;
	void OnDetach() override;
	void OnUpdate(f32 deltaTime) override;
	void OnImGuiRender() override;
	void OnEvent(Aquila::Events::Event &event) override;

	void PreloadTexturesRecursive(const std::string &basePath);

  private:
	enum class ViewMode { Grid, List };

	void RefreshDirectoryCache();
	void FilterDirectoryCache();

	void DrawToolbar(const ImVec2 &windowSize);
	void DrawBreadcrumb();
	void DrawFolderTree(FolderNode &node, std::string &selectedPath);
	void DrawContentGrid(f32 contentWidth);
	void DrawGridView(const std::vector<DirectoryEntry> &entries, f32 contentWidth);
	void DrawGridItem(const DirectoryEntry &entry, bool isSelected, f32 thumbnailSize);
	void DrawListView(const std::vector<DirectoryEntry> &entries);
	void DrawInfoPanel();

	void HandleContextMenus();
	void HandlePopups();

	void HandleSelectionRectangle();
	void HandleRangeSelection(const std::string &endPath);

	ImTextureID GetThumbnailForItem(const std::string &fullPath, const std::string &extension, bool isDirectory);
	void SetupDragDropPayload(const std::string &fullPath, const std::string &extension, const std::string &entryName);
	std::string GetFileTypeString(const std::string &path);
	std::string GetFileExtension(const std::string &filename);
	std::string GetParentPath(const std::string &path);
	void LoadContentBrowserTextures();
	std::string GetSlangTemplate(int templateIndex, const std::string &shaderName);

	void PasteFromClipboard(const std::string &destinationPath);
	bool CopyDirectoryRecursive(const std::string &srcPath, const std::string &dstPath);
	void ShowInExplorer(const std::string &virtualPath);

	Aquila::Core::Application &m_App;

	std::string m_CurrentPath;
	ViewMode m_ViewMode = ViewMode::Grid;
	f32 m_ThumbnailSize = 96.0f;
	f32 m_FolderPanelWidth = 200.0f;
	bool m_ShowItemContextMenu = false;
	bool m_ShowEmptySpaceContextMenu = false;
	bool m_ShowFolderContextMenu = false;
	std::string m_ClipboardPath;
	bool m_ClipboardIsCut = false;

	FolderNode m_RootNode;

	std::unordered_map<std::string, DirectoryCache> m_DirectoryCache;

	std::unordered_set<std::string> m_SelectedItems;
	std::unordered_set<std::string> m_ClipboardPaths;
	std::string m_LastSelectedItem;
	std::string m_ContextMenuTarget;

	bool m_IsSelectingWithRect = false;
	ImVec2 m_SelectionRectStart;
	ImVec2 m_SelectionRectEnd;
	std::unordered_map<std::string, ImVec2> m_ItemPositions;

	bool m_ShowDeletePopup = false;
	bool m_ShowRenamePopup = false;
	bool m_ShowCreateFolderPopup = false;
	bool m_ShowCreateMaterialPopup = false;
	bool m_ShowCreateSlangPopup = false;

	bool m_CreateSlangFocusRequested = false;
	bool m_RenameFocusRequested = false;
	bool m_CreateFolderFocusRequested = false;
	bool m_CreateMaterialFocusRequested = false;

	char m_SearchBuffer[Aquila::SharedConstants::NAME_MAX_LEN] = "";
	char m_RenameBuffer[Aquila::SharedConstants::NAME_MAX_LEN] = "";
	char m_NewFolderBuffer[Aquila::SharedConstants::NAME_MAX_LEN] = "";
	char m_NewMaterialNameBuffer[Aquila::SharedConstants::NAME_MAX_LEN] = "";
	char m_NewSlangNameBuffer[Aquila::SharedConstants::NAME_MAX_LEN] = "";
	char m_PendingShaderPath[Aquila::SharedConstants::PATH_MAX_LEN] = "";

	bool m_IconsLoaded = false;
	std::vector<Ref<Aquila::Graphics::Resources::Texture2D>> m_Textures;
	std::unordered_map<std::string, Ref<Aquila::Graphics::Resources::Texture2D>> m_ImagePreviews;
};

} // namespace Editor
