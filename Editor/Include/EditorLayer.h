
#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include "Engine/Controller.h"
#include "Engine/EditorCamera.h"
#include "UI/UI.h"


namespace Editor {
    class EditorLayer {
    public:
        static EditorLayer& Get() {
            static EditorLayer instance;
            return instance;
        }

        EditorLayer();
        ~EditorLayer();

        void OnAttach();
        void OnUpdate(VkCommandBuffer commandBuffer);
        void OnDetach();
    };
}



#endif //EDITORLAYER_H
