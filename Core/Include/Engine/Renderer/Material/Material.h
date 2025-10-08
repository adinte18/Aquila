#ifndef AQUILA_MATERIAL_H
#define AQUILA_MATERIAL_H

#include "Engine/Renderer/ShaderProgram.h"
#include "MaterialParameters.h"

namespace Engine {
namespace Material {
enum class BlendMode { Opaque, AlphaBlend, Additive, Multiply };

enum class CullMode { None, Front, Back };

struct RenderState {
  BlendMode m_BlendMode = BlendMode::Opaque;
  CullMode m_CullMode = CullMode::Back;
  bool m_DepthTest = true;
  bool m_DepthWrite = true;
  bool m_Wireframe = false;
  f32 m_LineWidth = 1.0f;
};

struct MaterialTemplate {
public:
  std::string name;
  Ref<Graphics::ShaderProgram> shader;
  RenderState renderState;
  std::unordered_map<std::string, MaterialParameter> properties;

  MaterialTemplate(const std::string &n) : name(n) {}

  void AddProperty(const MaterialParameter &prop) {
    properties[prop.m_Name] = prop;
  }

  bool HasProperty(const std::string &name) const {
    return properties.find(name) != properties.end();
  }

  const MaterialParameter *GetProperty(const std::string &name) const {
    auto it = properties.find(name);
    return it != properties.end() ? &it->second : nullptr;
  }
};

class Material {
private:
  Ref<MaterialTemplate> m_Template;
  std::unordered_map<std::string, MaterialParameter> m_Overrides;
  RenderState m_RenderState;
  bool m_IsDirty = true;
  bool m_PipelineDirty = true;
  bool m_DescriptorDirty = true;

  VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
  VkPipeline m_Pipeline = VK_NULL_HANDLE;

  std::unordered_map<std::string, Ref<Texture2D>> m_FallbackTextures;
  Device &m_Device;

  Unique<Buffer> m_MaterialUBO;
  void CreateMaterialUBO();

public:
  struct PBRMaterialUBO {
    alignas(16) glm::vec4 albedoColor;
    alignas(16) glm::vec4 emissiveColor;
    alignas(4) float metallic;
    alignas(4) float roughness;
    alignas(8) glm::vec2 padding;
  };

  struct UnlitMaterialUBO {
    alignas(16) glm::vec4 color;
  };

  std::string name;
  Material(const std::string &n, Ref<MaterialTemplate> tmpl, Device &device)
      : name(n), m_Template(tmpl), m_RenderState(tmpl->renderState),
        m_Device(device) {
    CreateMaterialUBO();
  }

  void UpdateMaterialUBO();
  Buffer *GetMaterialUBO() const { return m_MaterialUBO.get(); }

  void SetFloat(const std::string &name, f32 value);
  void SetVec2(const std::string &name, const vec2 &value);
  void SetVec3(const std::string &name, const vec3 &value);
  void SetVec4(const std::string &name, const vec4 &value);
  void SetColor(const std::string &name, const vec4 &color);
  void SetTexture(const std::string &name, Ref<Texture2D> texture);
  void SetParameter(const std::string &name, const ParameterValue &value);

  ParameterValue GetProperty(const std::string &name) const;
  Ref<Texture2D> GetTexture(const std::string &name) const;

  void SetFallbackTexture(const std::string &name, Ref<Texture2D> texture);

  void ReSetParameter(const std::string &name);
  void ResetAllProperties();
  void SetRenderState(const RenderState &state);

  const RenderState &GetRenderState() const;

  Ref<MaterialTemplate> GetTemplate() const;

  void SetTemplate(Ref<MaterialTemplate> tmpl);

  bool IsDirty() const;
  bool IsPipelineDirty() const;
  bool IsDescriptorDirty() const;
  void MarkDescriptorDirty();
  void MarkPipelineDirty();
  void MarkClean();
  void MarkDirty();

  VkDescriptorSet GetDescriptorSet() const;
  void SetDescriptorSet(VkDescriptorSet set);

  VkPipeline GetPipeline() const;
  void SetPipeline(VkPipeline pipeline);

  std::unordered_map<std::string, MaterialParameter> GetAllProperties() const;
};
} // namespace Material
} // namespace Engine

#endif