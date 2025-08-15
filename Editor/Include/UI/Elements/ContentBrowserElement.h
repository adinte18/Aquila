#ifndef CONTENT_ELEMENT_H
#define CONTENT_ELEMENT_H

#include "IElement.h"
#include <filesystem>
#include <fstream>
#include <string>


namespace Editor {
    namespace Elements {
        class ContentElement : public IElement {

            void CreateFolderPopup();
            void DeleteItemPopup();
            void RenameFolderPopup();
            
            void FolderActionsPopup();

            void ContentActionsPopup();

            void DrawFolderTree(const std::filesystem::path& basePath, std::filesystem::path& selectedPath);
            
            bool m_OpenDeletePopup = false; //https://github.com/ocornut/imgui/issues/331
            bool m_OpenRenamePopup = false;
            std::string m_CurrentlyHoveredPath;
            std::string m_RenameBuffer;
            std::vector<Ref<Engine::Texture2D>> m_Textures;
            public :
            void Draw() override;
            ContentElement();
        };
    }
}


#endif