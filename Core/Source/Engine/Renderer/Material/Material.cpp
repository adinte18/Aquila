#include "Engine/Renderer/Material/Material.h"

namespace Engine {
namespace Material {
void Material::SetFloat(const std::string &name, f32 value) {
  SetParameter(name, value);
}

void Material::SetVec2(const std::string &name, const vec2 &value) {
  SetParameter(name, value);
}

void Material::SetVec3(const std::string &name, const vec3 &value) {
  SetParameter(name, value);
}

void Material::SetVec4(const std::string &name, const vec4 &value) {
  SetParameter(name, value);
}

void Material::SetColor(const std::string &name, const vec4 &color) {
  SetParameter(name, color);
}

void Material::SetTexture(const std::string &name, Ref<Texture2D> texture) {
  SetParameter(name, texture);
}

void Material::CreateMaterialUBO() {

  size_t uboSize = 0;
  if (m_Template->name == "PBR") {
    uboSize = sizeof(PBRMaterialUBO);
  } else if (m_Template->name == "Unlit" || m_Template->name == "Transparent") {
    uboSize = sizeof(UnlitMaterialUBO);
  }

  if (uboSize > 0) {
    m_MaterialUBO =
        CreateUnique<Buffer>(m_Device, name + "_MaterialUBO", uboSize, 1,
                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    m_MaterialUBO->Map();
    UpdateMaterialUBO();
  }
}

void Material::UpdateMaterialUBO() {
  if (!m_MaterialUBO)
    return;

  if (m_Template->name == "PBR") {
    PBRMaterialUBO ubo{};
    ubo.albedoColor = std::get<vec4>(GetProperty("albedoColor"));
    ubo.emissiveColor = std::get<vec4>(GetProperty("emissiveColor"));
    ubo.metallic = std::get<f32>(GetProperty("metallic"));
    ubo.roughness = std::get<f32>(GetProperty("roughness"));

    m_MaterialUBO->Write(&ubo);
    m_MaterialUBO->Flush();
  } else if (m_Template->name == "Unlit" || m_Template->name == "Transparent") {
    UnlitMaterialUBO ubo{};
    ubo.color = std::get<vec4>(GetProperty("color"));

    m_MaterialUBO->Write(&ubo);
    m_MaterialUBO->Flush();
  }
}

void Material::SetParameter(const std::string &name,
                            const ParameterValue &value) {
  if (!m_Template->HasProperty(name))
    return;

  auto &prop = m_Overrides[name];
  if (m_Overrides.find(name) == m_Overrides.end()) {
    prop = *m_Template->GetProperty(name);
  }
  prop.m_Value = value;

  if (prop.m_Type == ParameterType::Texture2D) {
    MarkDescriptorDirty();
  } else {

    UpdateMaterialUBO();
    m_IsDirty = true;
  }
}

ParameterValue Material::GetProperty(const std::string &name) const {
  auto it = m_Overrides.find(name);
  if (it != m_Overrides.end()) {
    return it->second.m_Value;
  }
  auto *templateProp = m_Template->GetProperty(name);
  return templateProp ? templateProp->m_Value : ParameterValue{};
}

Ref<Texture2D> Material::GetTexture(const std::string &name) const {
  auto value = GetProperty(name);
  if (std::holds_alternative<Ref<Texture2D>>(value)) {
    auto tex = std::get<Ref<Texture2D>>(value);
    if (tex)
      return tex;
  }

  auto it = m_FallbackTextures.find(name);
  if (it != m_FallbackTextures.end() && it->second) {
    return it->second;
  }

  AQUILA_LOG_WARNING("Material {} has no texture or fallback for '{}'", name,
                     name);
  return nullptr;
}

void Material::SetFallbackTexture(const std::string &name,
                                  Ref<Texture2D> texture) {
  m_FallbackTextures[name] = texture;
}

void Material::ReSetParameter(const std::string &name) {
  m_Overrides.erase(name);
  m_IsDirty = true;
}

void Material::ResetAllProperties() {
  m_Overrides.clear();
  m_IsDirty = true;
}

void Material::SetRenderState(const RenderState &state) {
  m_RenderState = state;
  MarkPipelineDirty();
}

const RenderState &Material::GetRenderState() const { return m_RenderState; }

Ref<MaterialTemplate> Material::GetTemplate() const { return m_Template; }

void Material::SetTemplate(Ref<MaterialTemplate> tmpl) {
  m_Template = tmpl;
  m_RenderState = tmpl->renderState;
  m_IsDirty = true;
}

bool Material::IsDirty() const { return m_IsDirty; }
bool Material::IsPipelineDirty() const { return m_PipelineDirty; }
bool Material::IsDescriptorDirty() const { return m_DescriptorDirty; }

void Material::MarkClean() {
  m_IsDirty = false;
  m_PipelineDirty = false;
  m_DescriptorDirty = false;
}

void Material::MarkDescriptorDirty() {
  m_IsDirty = true;
  m_DescriptorDirty = true;
}

void Material::MarkPipelineDirty() {
  m_IsDirty = true;
  m_PipelineDirty = true;
}

void Material::MarkDirty() { m_IsDirty = true; }

VkDescriptorSet Material::GetDescriptorSet() const { return m_DescriptorSet; }
void Material::SetDescriptorSet(VkDescriptorSet set) { m_DescriptorSet = set; }

VkPipeline Material::GetPipeline() const { return m_Pipeline; }

void Material::SetPipeline(VkPipeline pipeline) { m_Pipeline = pipeline; }

std::unordered_map<std::string, MaterialParameter>
Material::GetAllProperties() const {
  std::unordered_map<std::string, MaterialParameter> props;

  for (const auto &[name, prop] : m_Template->properties) {
    MaterialParameter copy;
    copy.m_Name = prop.m_Name;
    copy.m_Type = prop.m_Type;
    copy.m_Value = prop.m_Value;
    copy.m_DefaultValue = prop.m_DefaultValue;
    copy.m_DisplayName = prop.m_DisplayName;
    copy.m_IsEditable = prop.m_IsEditable;
    copy.m_MinValue = prop.m_MinValue;
    copy.m_MaxValue = prop.m_MaxValue;
    copy.m_BindingIndex = prop.m_BindingIndex;

    props.emplace(name, std::move(copy));
  }

  for (const auto &[name, prop] : m_Overrides) {
    auto it = props.find(name);
    if (it != props.end()) {
      it->second.m_Value = prop.m_Value;
    }
  }
  return props;
}
} // namespace Material
} // namespace Engine