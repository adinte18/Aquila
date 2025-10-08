#pragma once
#include "Engine/Renderer/Material/Material.h"

struct MaterialComponent {
  Ref<Engine::Material::Material> material;

  MaterialComponent() = default;
  MaterialComponent(const Ref<Engine::Material::Material> &mat)
      : material(mat) {}

  Ref<Engine::Material::Material> GetMaterial() const { return material; }
  void SetMaterial(Ref<Engine::Material::Material> mat) { material = mat; }
};
