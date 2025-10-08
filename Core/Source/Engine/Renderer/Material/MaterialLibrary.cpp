#include "Engine/Renderer/Material/MaterialLibrary.h"

namespace Engine {
namespace Material {

static Ref<DescriptorSetLayout> globalLayout = nullptr;

void MaterialLibrary::Initialize() {
  CreateFallbackTextures();
  CreateShaderPrograms();
  CreateDefaultTemplates();
  CreateDefaultMaterials();
}

void MaterialLibrary::Shutdown() {
  AQUILA_LOG_DEBUG("MaterialLibrary::Shutdown() - Start");

  m_DefaultMaterial.reset();
  m_FallbackMaterial.reset();
  m_Materials.clear();

  AQUILA_LOG_DEBUG("Clearing {} templates", m_Templates.size());
  m_Templates.clear();

  AQUILA_LOG_DEBUG("Cleaning up {} shader programs", m_ShaderPrograms.size());
  for (auto &[name, shader] : m_ShaderPrograms) {
    if (shader) {
      AQUILA_LOG_DEBUG("Manually cleaning up shader: {}", name);
      shader->Cleanup();
    }
  }
  m_ShaderPrograms.clear();

  m_WhiteTexture.reset();
  m_NormalTexture.reset();
  m_BlackTexture.reset();

  AQUILA_LOG_DEBUG("MaterialLibrary::Shutdown() - Complete");
}

void MaterialLibrary::CreateShaderPrograms() {

  auto pbrShader = CreateRef<Graphics::ShaderProgram>(m_Device, "PBRShader");
  pbrShader->AddStage(VK_SHADER_STAGE_VERTEX_BIT,
                      std::string(SHADERS_PATH) + "/AqPBRvert.spv");
  pbrShader->AddStage(VK_SHADER_STAGE_FRAGMENT_BIT,
                      std::string(SHADERS_PATH) + "/AqPBRfrag.spv");

  pbrShader->m_DescriptorSetLayout =
      DescriptorSetLayout::Builder(m_Device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .addBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

  m_ShaderPrograms["PBR"] = pbrShader;

  auto unlitShader =
      CreateRef<Graphics::ShaderProgram>(m_Device, "UnlitShader");
  unlitShader->AddStage(VK_SHADER_STAGE_VERTEX_BIT,
                        std::string(SHADERS_PATH) + "/AqUnlitVert.spv");
  unlitShader->AddStage(VK_SHADER_STAGE_FRAGMENT_BIT,
                        std::string(SHADERS_PATH) + "/AqUnlitFrag.spv");

  unlitShader->m_DescriptorSetLayout =
      DescriptorSetLayout::Builder(m_Device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_FRAGMENT_BIT)
          .build();

  m_ShaderPrograms["Unlit"] = unlitShader;
  m_ShaderPrograms["Transparent"] = unlitShader;
}

void MaterialLibrary::CreateFallbackTextures() {
  m_WhiteTexture = CreateRef<Texture2D>(m_Device, "Fallback_White");
  m_WhiteTexture->UseFallbackTextures(Texture2D::TextureType::Albedo);

  m_NormalTexture = CreateRef<Texture2D>(m_Device, "Fallback_Normal");
  m_NormalTexture->UseFallbackTextures(Texture2D::TextureType::Normal);

  m_BlackTexture = CreateRef<Texture2D>(m_Device, "Fallback_Black");
  m_BlackTexture->UseFallbackTextures(Texture2D::TextureType::Albedo);
}

Ref<Texture2D>
MaterialLibrary::GetFallbackTextureForType(const std::string &propName) {
  if (propName.find("albedo") != std::string::npos ||
      propName.find("Albedo") != std::string::npos ||
      propName.find("diffuse") != std::string::npos ||
      propName.find("mainTexture") != std::string::npos) {
    return m_WhiteTexture;
  }
  if (propName.find("normal") != std::string::npos ||
      propName.find("Normal") != std::string::npos) {
    return m_NormalTexture;
  }
  return m_WhiteTexture;
}

void MaterialLibrary::CreateDefaultTemplates() {

  auto unlitTemplate = CreateRef<MaterialTemplate>("Unlit");
  unlitTemplate->shader = m_ShaderPrograms["Unlit"];

  unlitTemplate->AddProperty(
      MaterialParameter("color", ParameterType::Color, glm::vec4(1.0f)));

  auto mainTexParam = MaterialParameter("mainTexture", ParameterType::Texture2D,
                                        Ref<Texture2D>());
  mainTexParam.m_BindingIndex = 0;
  mainTexParam.m_DisplayName = "Texture";
  unlitTemplate->AddProperty(mainTexParam);

  unlitTemplate->renderState.m_DepthTest = true;
  RegisterTemplate(unlitTemplate);

  auto pbrTemplate = CreateRef<MaterialTemplate>("PBR");
  pbrTemplate->shader = m_ShaderPrograms["PBR"];

  pbrTemplate->AddProperty(MaterialParameter(
      "albedoColor", ParameterType::Color, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)));

  auto albedoMapParam = MaterialParameter("albedoMap", ParameterType::Texture2D,
                                          Ref<Texture2D>());
  albedoMapParam.m_BindingIndex = 0;
  albedoMapParam.m_DisplayName = "Albedo Map";
  pbrTemplate->AddProperty(albedoMapParam);

  pbrTemplate->AddProperty(
      MaterialParameter("metallic", ParameterType::Float, 0.0f));
  pbrTemplate->AddProperty(
      MaterialParameter("roughness", ParameterType::Float, 0.5f));

  auto normalMapParam = MaterialParameter("normalMap", ParameterType::Texture2D,
                                          Ref<Texture2D>());
  normalMapParam.m_BindingIndex = 1;
  normalMapParam.m_DisplayName = "Normal Map";
  pbrTemplate->AddProperty(normalMapParam);

  auto aoMapParam =
      MaterialParameter("aoMap", ParameterType::Texture2D, Ref<Texture2D>());
  aoMapParam.m_BindingIndex = 2;
  aoMapParam.m_DisplayName = "AO Map";
  pbrTemplate->AddProperty(aoMapParam);

  pbrTemplate->AddProperty(
      MaterialParameter("emissiveColor", ParameterType::Color,
                        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)));

  auto emissiveMapParam = MaterialParameter(
      "emissiveMap", ParameterType::Texture2D, Ref<Texture2D>());
  emissiveMapParam.m_BindingIndex = 3;
  emissiveMapParam.m_DisplayName = "Emissive Map";
  pbrTemplate->AddProperty(emissiveMapParam);

  pbrTemplate->properties["albedoColor"].m_DisplayName = "Albedo Color";
  pbrTemplate->properties["albedoMap"].m_DisplayName = "Albedo Map";
  pbrTemplate->properties["metallic"].m_DisplayName = "Metallic";
  pbrTemplate->properties["metallic"].m_MinValue = 0.0f;
  pbrTemplate->properties["metallic"].m_MaxValue = 1.0f;
  pbrTemplate->properties["roughness"].m_DisplayName = "Roughness";
  pbrTemplate->properties["roughness"].m_MinValue = 0.0f;
  pbrTemplate->properties["roughness"].m_MaxValue = 1.0f;
  pbrTemplate->properties["normalMap"].m_DisplayName = "Normal Map";
  pbrTemplate->properties["aoMap"].m_DisplayName = "AO Map";
  pbrTemplate->properties["emissiveColor"].m_DisplayName = "Emissive Color";
  pbrTemplate->properties["emissiveMap"].m_DisplayName = "Emissive Map";

  RegisterTemplate(pbrTemplate);

  auto transparentTemplate = CreateRef<MaterialTemplate>("Transparent");
  transparentTemplate->shader = m_ShaderPrograms["Transparent"];

  transparentTemplate->AddProperty(MaterialParameter(
      "color", ParameterType::Color, glm::vec4(1.0f, 1.0f, 1.0f, 0.5f)));
  transparentTemplate->AddProperty(MaterialParameter(
      "mainTexture", ParameterType::Texture2D, Ref<Texture2D>()));

  transparentTemplate->properties["color"].m_DisplayName = "Color";
  transparentTemplate->properties["mainTexture"].m_DisplayName = "Texture";

  transparentTemplate->renderState.m_BlendMode = BlendMode::AlphaBlend;
  transparentTemplate->renderState.m_DepthWrite = false;
  RegisterTemplate(transparentTemplate);
}

void MaterialLibrary::CreateDefaultMaterials() {
  m_FallbackMaterial = CreateMaterial("FallbackMaterial", "Unlit");
  m_FallbackMaterial->SetColor("color", glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
  m_FallbackMaterial->SetTexture("mainTexture", m_WhiteTexture);
  m_FallbackMaterial->MarkDirty();

  AQUILA_LOG_DEBUG("FallbackMaterial mainTexture: {}",
                   m_FallbackMaterial->GetTexture("mainTexture") ? "SET"
                                                                 : "NULL");

  m_DefaultMaterial = CreateMaterial("DefaultMaterial", "PBR");
  m_DefaultMaterial->SetColor("albedoColor", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
  m_DefaultMaterial->SetColor("emissiveColor",
                              glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
  m_DefaultMaterial->SetFloat("metallic", 0.0f);
  m_DefaultMaterial->SetFloat("roughness", 0.0f);
  m_DefaultMaterial->SetTexture("albedoMap", m_WhiteTexture);
  m_DefaultMaterial->SetTexture("normalMap", m_NormalTexture);
  m_DefaultMaterial->SetTexture("aoMap", m_WhiteTexture);
  m_DefaultMaterial->SetTexture("emissiveMap", m_BlackTexture);
  m_DefaultMaterial->MarkDirty();

  AQUILA_LOG_DEBUG("DefaultMaterial albedoMap: {}",
                   m_DefaultMaterial->GetTexture("albedoMap") ? "SET" : "NULL");
}

void MaterialLibrary::RegisterTemplate(Ref<MaterialTemplate> tmpl) {
  m_Templates[tmpl->name] = tmpl;
}

Ref<MaterialTemplate> MaterialLibrary::GetTemplate(const std::string &name) {
  auto it = m_Templates.find(name);
  return it != m_Templates.end() ? it->second : nullptr;
}

Ref<Material> MaterialLibrary::CreateMaterial(const std::string &name,
                                              const std::string &templateName) {
  auto tmpl = GetTemplate(templateName);
  if (!tmpl) {
    AQUILA_LOG_ERROR("Template '{}' not found, using fallback", templateName);
    return m_FallbackMaterial;
  }

  auto mat = CreateRef<Material>(name, tmpl, m_Device);

  for (const auto &[propName, prop] : tmpl->properties) {
    if (prop.m_Type == ParameterType::Texture2D) {
      auto fallback = GetFallbackTextureForType(propName);
      mat->SetFallbackTexture(propName, fallback);

      if (!std::holds_alternative<Ref<Texture2D>>(prop.m_Value) ||
          !std::get<Ref<Texture2D>>(prop.m_Value)) {
        mat->SetTexture(propName, fallback);
      }
    }
  }

  mat->MarkDirty();
  m_Materials[name] = mat;
  return mat;
}

Ref<Material> MaterialLibrary::GetMaterial(const std::string &name) {
  auto it = m_Materials.find(name);
  return it != m_Materials.end() ? it->second : m_DefaultMaterial;
}
} // namespace Material
} // namespace Engine