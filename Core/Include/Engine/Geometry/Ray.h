#ifndef AQUILA_GEOMETRY_RAY_H
#define AQUILA_GEOMETRY_RAY_H

#include "Utilities/Math.h"
#include <glm/glm.hpp>
#include <limits>

namespace Geometry {

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;

  Ray(const glm::vec3 &o, const glm::vec3 &d)
      : origin(o), direction(normalize(d)) {}

  glm::vec3 GetPoint(float t) const { return origin + direction * t; }

  bool IntersectAABB(const glm::vec3 &aabbMin, const glm::vec3 &aabbMax,
                     float &distance) const {
    glm::vec3 invDir = 1.0f / direction;
    glm::vec3 t1 = (aabbMin - origin) * invDir;
    glm::vec3 t2 = (aabbMax - origin) * invDir;

    glm::vec3 tMin = glm::min(t1, t2);
    glm::vec3 tMax = glm::max(t1, t2);

    float tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
    float tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);

    if (tNear > tFar || tFar < 0.0f) {
      return false;
    }

    distance = tNear > 0.0f ? tNear : tFar;
    return true;
  }

  bool IntersectSphere(const glm::vec3 &center, float radius,
                       float &distance) const {
    glm::vec3 oc = origin - center;
    float a = glm::dot(direction, direction);
    float b = 2.0f * glm::dot(oc, direction);
    float c = glm::dot(oc, oc) - radius * radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
      return false;
    }

    float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
    float t2 = (-b + sqrt(discriminant)) / (2.0f * a);

    if (t1 > 0) {
      distance = t1;
      return true;
    } else if (t2 > 0) {
      distance = t2;
      return true;
    }

    return false;
  }

  bool IntersectTriangle(const glm::vec3 &v0, const glm::vec3 &v1,
                         const glm::vec3 &v2, float &distance) const {
    glm::vec3 edge1, edge2, h, s, q;
    float a, f, u, v;

    edge1 = v1 - v0;
    edge2 = v2 - v0;
    h = glm::cross(direction, edge2);
    a = glm::dot(edge1, h);

    if (a > -Utility::Math::EPSILON && a < Utility::Math::EPSILON) {
      return false;
    }

    f = 1.0f / a;
    s = origin - v0;
    u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) {
      return false;
    }

    q = glm::cross(s, edge1);
    v = f * glm::dot(direction, q);

    if (v < 0.0f || u + v > 1.0f) {
      return false;
    }

    float t = f * glm::dot(edge2, q);

    if (t > Utility::Math::EPSILON) {
      distance = t;
      return true;
    }

    return false;
  }

  bool IntersectLine(const glm::vec3 &lineStart, const glm::vec3 &lineEnd,
                     float threshold, float &distance) const {
    glm::vec3 lineDir = lineEnd - lineStart;
    glm::vec3 rayToLine = lineStart - origin;

    glm::vec3 cross1 = glm::cross(direction, lineDir);
    glm::vec3 cross2 = glm::cross(rayToLine, lineDir);

    float denominator = glm::dot(cross1, cross1);

    if (denominator < 1e-6f) {
      return false;
    }

    float t = glm::dot(cross2, cross1) / denominator;
    float u = glm::dot(glm::cross(rayToLine, direction), cross1) / denominator;

    if (t < 0.0f || u < 0.0f || u > 1.0f) {
      return false;
    }

    glm::vec3 closestPointOnRay = origin + t * direction;
    glm::vec3 closestPointOnLine = lineStart + u * lineDir;

    float dist = glm::length(closestPointOnRay - closestPointOnLine);

    if (dist <= threshold) {
      distance = t;
      return true;
    }

    return false;
  }

  bool IntersectPlane(const glm::vec3 &planeNormal, float planeDistance,
                      float &distance) const {
    float denom = glm::dot(planeNormal, direction);

    if (abs(denom) < 1e-6f) {
      return false;
    }

    float t = (planeDistance - glm::dot(planeNormal, origin)) / denom;

    if (t >= 0) {
      distance = t;
      return true;
    }

    return false;
  }

  bool IntersectCylinder(const glm::vec3 &cylinderStart,
                         const glm::vec3 &cylinderEnd, float radius,
                         float &distance) const {
    glm::vec3 cylinderAxis = glm::normalize(cylinderEnd - cylinderStart);
    glm::vec3 toRayOrigin = origin - cylinderStart;

    glm::vec3 rayDirPerp =
        direction - glm::dot(direction, cylinderAxis) * cylinderAxis;
    glm::vec3 toRayOriginPerp =
        toRayOrigin - glm::dot(toRayOrigin, cylinderAxis) * cylinderAxis;

    float a = glm::dot(rayDirPerp, rayDirPerp);
    float b = 2.0f * glm::dot(toRayOriginPerp, rayDirPerp);
    float c = glm::dot(toRayOriginPerp, toRayOriginPerp) - radius * radius;

    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
      return false;
    }

    float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
    float t2 = (-b + sqrt(discriminant)) / (2.0f * a);

    auto isWithinCylinder = [&](float t) -> bool {
      glm::vec3 point = GetPoint(t);
      glm::vec3 toPoint = point - cylinderStart;
      float projection = glm::dot(toPoint, cylinderAxis);
      float cylinderLength = glm::length(cylinderEnd - cylinderStart);
      return projection >= 0.0f && projection <= cylinderLength;
    };

    if (t1 > 0 && isWithinCylinder(t1)) {
      distance = t1;
      return true;
    } else if (t2 > 0 && isWithinCylinder(t2)) {
      distance = t2;
      return true;
    }

    return false;
  }

  glm::vec3 ClosestPointTo(const glm::vec3 &point) const {
    glm::vec3 toPoint = point - origin;
    float t = glm::dot(toPoint, direction);
    return GetPoint(glm::max(0.0f, t));
  }

  float DistanceToPoint(const glm::vec3 &point) const {
    return glm::length(point - ClosestPointTo(point));
  }
};

inline Ray ScreenToWorldRay(const glm::vec2 &screenPos,
                            const glm::vec2 &viewportSize,
                            const glm::mat4 &viewMatrix,
                            const glm::mat4 &projMatrix) {

  glm::vec2 ndc;
  ndc.x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
  ndc.y = 1.0f - (2.0f * screenPos.y) / viewportSize.y;

  glm::mat4 invViewProj = glm::inverse(projMatrix * viewMatrix);

  glm::vec4 nearPoint = glm::vec4(ndc.x, ndc.y, 0.0f, 1.0f);
  glm::vec4 farPoint = glm::vec4(ndc.x, ndc.y, 1.0f, 1.0f);

  glm::vec4 worldNear = invViewProj * nearPoint;
  glm::vec4 worldFar = invViewProj * farPoint;

  worldNear /= worldNear.w;
  worldFar /= worldFar.w;

  glm::vec3 rayOrigin = glm::vec3(worldNear);
  glm::vec3 rayDirection =
      glm::normalize(glm::vec3(worldFar) - glm::vec3(worldNear));

  return Ray(rayOrigin, rayDirection);
}

inline Ray CameraRay(const glm::vec2 &screenPos, const glm::vec2 &viewportSize,
                     const glm::vec3 &cameraPos, const glm::vec3 &cameraForward,
                     const glm::vec3 &cameraUp, const glm::vec3 &cameraRight,
                     float fov, float aspectRatio) {

  float x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
  float y = 1.0f - (2.0f * screenPos.y) / viewportSize.y;

  float tanHalfFov = tan(glm::radians(fov) / 2.0f);
  glm::vec3 rayDir = glm::normalize(
      cameraForward + (x * aspectRatio * tanHalfFov) * cameraRight +
      (y * tanHalfFov) * cameraUp);

  return Ray(cameraPos, rayDir);
}

} // namespace Geometry

#endif