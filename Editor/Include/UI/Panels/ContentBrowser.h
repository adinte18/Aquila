#ifndef CONTENT__H
#define CONTENT__H

#include "UI/Panels/IPanel.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace Editor {
namespace Panels {
class Content : public IPanel {

  void CreateFolderPopup();
  void DeleteItemPopup();
  void RenameFolderPopup();

  void FolderActionsPopup();

  void ContentActionsPopup();

  void DrawFolderTree(const std::string &basePath, std::string &selectedPath);
  std::string GetFileExtension(const std::string &filename);
  std::string GetParentPath(const std::string &path);

  bool m_OpenDeletePopup = false; // https://github.com/ocornut/imgui/issues/331
  bool m_OpenRenamePopup = false;
  std::string m_CurrentlyHoveredPath;
  std::string m_RenameBuffer;
  std::vector<Ref<Engine::Texture2D>> m_Textures;

public:
  void Draw() override;

  Content();
};
} // namespace Panels
} // namespace Editor

#endif