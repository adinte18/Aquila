
#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include "Engine/Controller.h"
#include "Engine/EditorCamera.h"
#include "UI/UI.h"
#include "Utilities/Singleton.h"


namespace Editor {
    class EditorLayer : public Utility::Singleton<EditorLayer> {
        friend class Utility::Singleton<EditorLayer>;
        
        public:
        EditorLayer();
        ~EditorLayer();

        void OnAttach();
        void RenderUI(VkCommandBuffer commandBuffer);
        void OnDetach();
    };
}



#endif //EDITORLAYER_H
