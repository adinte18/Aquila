#ifndef PROPERTIES_ELEM_H
#define PROPERTIES_ELEM_H

#include "Elements/IElement.h"
#include "Scene/Scene.h"

namespace Editor::UIManagement {
    class PropertiesElement : public IElement{
        private:

        void DrawComponent_Transform(entt::registry& registry, entt::entity entity);
        void DrawComponent_Metadata(entt::registry& registry, entt::entity entity);
        void DrawComponent_Mesh(entt::registry& registry, entt::entity entity);
        void DrawComponent_Camera(entt::registry& registry, entt::entity entity);

        public :
        void Draw(void* data) override;
    };
}

#endif