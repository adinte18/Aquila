
#ifndef EDITORLAYER_H
#define EDITORLAYER_H

#include "Engine/EditorCamera.h"
#include "UIManagement/UI.h"

#include "Engine/Core.h"

namespace Editor {

    class EditorLayer {
    public:
        EditorLayer() {
            OnStart();
        }

        ~EditorLayer() {
            OnEnd();
        }

        void OnStart();
        void OnUpdate();
        void OnEnd();
       

    private:
        // Unique pointer for editor camera
        Unique<Engine::EditorCamera> m_EditorCamera;

        // UI Manager
        Unique<UIManager> m_UI;
    };
}



#endif //EDITORLAYER_H
