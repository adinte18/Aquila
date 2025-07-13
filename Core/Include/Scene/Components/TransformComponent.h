#ifndef TRANSFORM_COMPONENT_H
#define TRANSFORM_COMPONENT_H

#include "AquilaCore.h"

struct TransformComponent {
public:
    TransformComponent(const glm::vec3& position = glm::vec3{0.f},
                       const glm::quat& rotation = glm::quat{1.f, 0.f, 0.f, 0.f},
                       const glm::vec3& scale = glm::vec3{1.f})
        : m_LocalPosition(position), m_LocalRotation(rotation), m_LocalScale(scale), m_WorldMatrix(1.0f) {}

    void SetLocalPosition(const glm::vec3& position) { m_LocalPosition = position; }
    void SetLocalRotation(const glm::quat& rotation) { m_LocalRotation = rotation; }
    void SetLocalScale(const glm::vec3& scale) { m_LocalScale = scale; }

    glm::vec3& GetLocalPosition() { return m_LocalPosition; }
    glm::quat& GetLocalRotation() { return m_LocalRotation; }
    glm::vec3& GetLocalScale() { return m_LocalScale; }

    [[nodiscard]] glm::mat4 GetLocalTransformMatrix() const {
        glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), m_LocalPosition);
        glm::mat4 rotationMatrix = glm::toMat4(m_LocalRotation);
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), m_LocalScale);
        return translationMatrix * rotationMatrix * scaleMatrix;
    }

    void UpdateWorldMatrix(const glm::mat4& parentMatrix = glm::mat4(1.0f)) {
        m_WorldMatrix = parentMatrix * GetLocalTransformMatrix();
    }

    const glm::mat4& GetWorldMatrix() const { return m_WorldMatrix; }

    [[nodiscard]] glm::mat3 GetNormalMatrix() const {
        return glm::mat3_cast(m_LocalRotation);
    }

private:
    glm::vec3 m_LocalPosition{0.f};
    glm::quat m_LocalRotation{1.f, 0.f, 0.f, 0.f};
    glm::vec3 m_LocalScale{1.f};
    glm::mat4 m_WorldMatrix{1.f};
};

#endif