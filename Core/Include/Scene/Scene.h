#ifndef AQUILA_SCENE_H
#define AQUILA_SCENE_H

#include "AquilaCore.h"
#include "entt.h"
#include "json.hpp"

namespace Engine {
    class Entity;
    class EntityManager;
    class SceneGraph;

    class AquilaScene  {
    public : 

        explicit AquilaScene();
        explicit AquilaScene(const std::string& name);

        AquilaScene(const AquilaScene&) = delete;
        AquilaScene& operator=(const AquilaScene&) = delete;
        AquilaScene(AquilaScene&&) = delete;
        AquilaScene& operator=(AquilaScene&&) = delete;
        
        virtual ~AquilaScene();

        virtual void OnStart();
        // virtual void OnClean();
        // virtual void OnUpdate(const DeltaTime& timeStep);
        // virtual void OnUpdate(const DeltaTime& timeStep, EditorCamera& camera);
        // virtual void Render();

        entt::registry& GetRegistry();
        EntityManager* GetEntityManager();
        SceneGraph* GetSceneGraph();
        const std::string& GetSceneName();

        const UUID GetHandle() const;

        bool Serialize(const std::string& filepath);
        bool Deserialize(const std::string& filepath);
        
    protected:
        std::string m_SceneName;
        Unique<EntityManager> m_EntityManager;
        Unique<SceneGraph> m_SceneGraph;
 
    private:
        UUID m_SceneID; // scene handle

        friend class Entity;
        friend class EntityManager;
        friend class SceneGraph;
    };
}

#endif