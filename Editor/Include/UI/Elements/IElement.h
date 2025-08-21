#ifndef UICOMPONENT_H
#define UICOMPONENT_H

#include "UI/UIConfig.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/EventBus.h"
#include "Engine/Controller.h"

namespace Editor {
    namespace Elements {
        class IElement {
        public:
            IElement() = default;
            virtual ~IElement() = default;

            virtual void Draw() = 0;

            // lifecycle
            virtual void OnAttach() { SetOpen(true); }
            virtual void OnDetach() { SetOpen(false); }

            bool IsOpen() const { return m_Open; }
            void SetOpen(bool open) { m_Open = open; }

            void CreatePopup(ImVec2 windowSize) {
                ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

                ImVec2 ViewportPos = ImGui::GetMainViewport()->GetCenter();
                ImVec2 WindowPos = ImVec2(ViewportPos.x - windowSize.x * 0.5f, ViewportPos.y - windowSize.y * 0.5f);

                ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Once);
            }

        protected:
            bool m_Open = false;
        };
    }
}

#endif // UICOMPONENT_H
