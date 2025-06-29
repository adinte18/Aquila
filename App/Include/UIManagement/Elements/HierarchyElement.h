#ifndef HIERARCHY_ELEMENT_H
#define HIERARCHY_ELEMENT_H

#include "IElement.h"
#include "Scene/Scene.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/SceneGraph.h"

namespace Editor::UIManagement {
    class HierarchyElement : public IElement {
        private:
        void DisplayHierarchy(Engine::AquilaScene* scene);
        void DisplayEntityNode(Engine::AquilaScene* scene, entt::entity entity);
        void ToggleVisibility(entt::registry& registry, entt::entity& entity, bool& value);

        void PopupMenu();
        void Menu();

        public:
        void Draw(void* data) override;
    };
}

#endif