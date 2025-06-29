#include "Scene/Entity.h"

namespace Engine {

    // by design both are set on entity creation
    const UUID& AqEntity::GetUUID() const{
        return TryGetComponent<MetadataComponent>()->ID;
    }

    const std::string& AqEntity::GetName() const {
        return TryGetComponent<MetadataComponent>()->Name;
    }

    void AqEntity::Kill()
    {
        AQUILA_CORE_ASSERT(m_Scene);
        m_Scene->GetRegistry().destroy(m_EntityHandle);
    }

    bool AqEntity::IsValid() const {
        AQUILA_CORE_ASSERT(m_Scene);
        return m_Scene && m_Scene->GetRegistry().valid(m_EntityHandle);
    }


}