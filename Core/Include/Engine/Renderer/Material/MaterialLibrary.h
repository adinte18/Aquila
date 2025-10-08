#ifndef AQUILA_MATERIAL_LIB_H
#define AQUILA_MATERIAL_LIB_H

#include "Engine/Renderer/Texture2D.h"
#include "Material.h"
#include "MaterialParameters.h"
#include "Platform/PrimitiveTypes.h"

namespace Engine {
namespace Material {

class MaterialLibrary {
private:
  Device &m_Device;
  std::unordered_map<std::string, Ref<MaterialTemplate>> m_Templates;
  std::unordered_map<std::string, Ref<Material>> m_Materials;
  std::unordered_map<std::string, Ref<Graphics::ShaderProgram>>
      m_ShaderPrograms;

  Ref<Material> m_DefaultMaterial;
  Ref<Material> m_FallbackMaterial;

  Ref<Texture2D> m_WhiteTexture;
  Ref<Texture2D> m_NormalTexture;
  Ref<Texture2D> m_BlackTexture;

  void CreateFallbackTextures();
  void CreateShaderPrograms();
  void CreateDefaultTemplates();
  void CreateDefaultMaterials();

public:
  MaterialLibrary(Device &device) : m_Device(device) {}

  void Initialize();
  void Shutdown();
  Ref<Texture2D> GetFallbackTextureForType(const std::string &propName);

  void RegisterTemplate(Ref<MaterialTemplate> tmpl);
  Ref<MaterialTemplate> GetTemplate(const std::string &name);

  Ref<Material> CreateMaterial(const std::string &name,
                               const std::string &templateName);
  Ref<Material> GetMaterial(const std::string &name);

  Ref<Material> GetDefaultMaterial() const { return m_DefaultMaterial; }
  Ref<Material> GetFallbackMaterial() const { return m_FallbackMaterial; }

  const auto &GetAllTemplates() const { return m_Templates; }
  const auto &GetAllMaterials() const { return m_Materials; }
};

} // namespace Material
} // namespace Engine

#endif