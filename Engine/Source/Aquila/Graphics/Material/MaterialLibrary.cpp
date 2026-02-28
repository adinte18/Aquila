#include "Aquila/Graphics/Material/MaterialLibrary.h"

namespace Aquila::Graphics::Material {

using namespace Aquila::Graphics::RenderingPipeline;
using namespace Aquila::Graphics::Resources;

void MaterialLibrary::Initialize() {
	Shader::ShaderCompiler::Initialize();
	EnableShaderHotReload(true);
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

	Shader::ShaderCompiler::Shutdown();
	AQUILA_LOG_DEBUG("MaterialLibrary::Shutdown() - Complete");
}

void MaterialLibrary::CreateShaderPrograms() {
	std::string errorLog;

	auto pbrShader = CreateRef<Shader::ShaderProgram>(m_Device, "PBR");
	pbrShader->SetShaderType(ShaderType::Standard);
	pbrShader->SetTargetFormats(Helpers::PipelineRenderingFormats::GBuffer());

	if (!pbrShader->AddStageFromSlang(std::string(IMMUTABLE_SHADERS_PATH) + "GBuffer.slang", errorLog)) {
		AQUILA_LOG_ERROR("Failed to compile PBR shader: {}", errorLog);
	} else {
		pbrShader->Reflect();
	}

	if (pbrShader->GetDescriptorSetLayout() == nullptr) {
		AQUILA_LOG_WARNING("PBR shader reflection failed, using fallback layout");
		pbrShader->m_DescriptorSetLayout =
			DescriptorSetLayout::Builder(m_Device)
				.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();
	}

	m_ShaderPrograms["PBR"] = pbrShader;
	WatchShader(pbrShader);

	auto unlitShader = CreateRef<Shader::ShaderProgram>(m_Device, "Unlit");
	unlitShader->SetShaderType(ShaderType::Standard);
	unlitShader->SetTargetFormats(Helpers::PipelineRenderingFormats::GBuffer());

	if (!unlitShader->AddStageFromSlang(std::string(IMMUTABLE_SHADERS_PATH) + "Unlit.slang", errorLog)) {
		AQUILA_LOG_ERROR("Failed to compile Unlit shader: {}", errorLog);
	} else {
		unlitShader->Reflect();
	}

	if (unlitShader->GetDescriptorSetLayout() == nullptr) {
		AQUILA_LOG_WARNING("Unlit shader reflection failed, using fallback layout");
		unlitShader->m_DescriptorSetLayout =
			DescriptorSetLayout::Builder(m_Device)
				.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();
	}

	m_ShaderPrograms["Unlit"] = unlitShader;
	WatchShader(unlitShader);
}

void MaterialLibrary::WatchShader(const Ref<Shader::ShaderProgram> &shader) {
	if (!shader || shader->GetSlangPath().empty()) {
		return;
	}

	m_ShaderWatcher.WatchSlangFile(shader->GetSlangPath(), shader->m_Name);
}

void MaterialLibrary::EnableShaderHotReload(bool enable) {
	m_ShaderWatcher.Enable(enable);
}

void MaterialLibrary::TickHotReload() {
	if (!m_ShaderWatcher.IsEnabled()) {
		return;
	}

	const auto changed = m_ShaderWatcher.CheckForChanges();
	if (changed.empty()) {
		return;
	}

	for (const auto &programName : changed) {
		auto it = m_ShaderPrograms.find(programName);
		if (it == m_ShaderPrograms.end()) {
			AQUILA_LOG_WARNING("ShaderWatcher reported change for unknown program '{}'", programName);
			continue;
		}

		auto &shader = it->second;
		std::string errorLog;

		Ref<RenderingPipeline::DescriptorSetLayout> newLayout;
		if (!shader->Reload(errorLog, newLayout)) {
			AQUILA_LOG_ERROR("Hot-reload failed for '{}': {}", programName, errorLog);
			continue;
		}

		shader->CommitNewLayout(std::move(newLayout));

		AQUILA_LOG_INFO("Hot-reload succeeded for '{}', re-syncing materials", programName);
		for (auto &[matName, mat] : m_Materials) {
			if (mat && mat->GetShader() == shader) {
				mat->SyncPropertiesFromShader();
				mat->MarkPipelineDirty();
				mat->MarkDescriptorDirty();

				if (m_OnShaderReloaded) {
					m_OnShaderReloaded(mat);
				}
			}
		}
	}
}

Ref<Material> MaterialLibrary::CreateMaterialFromShader(const std::string &name,
														const Ref<Shader::ShaderProgram> &shader) {
	if (!shader) {
		AQUILA_LOG_ERROR("Cannot create material '{}': null shader", name);
		return nullptr;
	}

	auto tmpl = CreateRef<MaterialTemplate>(name + "_template");
	tmpl->shader = shader;
	tmpl->shaderType = ShaderType::Custom;
	RegisterTemplate(tmpl);

	auto mat = CreateRef<Material>(m_Device, name, tmpl);

	mat->SyncPropertiesFromShader();

	for (const auto &[propName, prop] : tmpl->properties) {
		if (prop.m_Type == ParameterType::Texture2D) {
			auto fallback = GetFallbackTextureForType(propName);
			mat->SetFallbackTexture(propName, fallback);
			mat->SetParameter(propName, fallback);
		}
	}

	mat->CreateMaterialUBO();

	mat->MarkPipelineDirty();
	mat->MarkDirty();

	m_ShaderPrograms[shader->m_Name] = shader;
	m_Materials[name] = mat;
	WatchShader(shader);

	if (m_OnMaterialCreated) {
		m_OnMaterialCreated(mat);
	}

	return mat;
}

Ref<Material> MaterialLibrary::CreateMaterial(const std::string &name, const std::string &templateName) {
	auto tmpl = GetTemplate(templateName);
	if (!tmpl) {
		AQUILA_LOG_ERROR("Template '{}' not found, using fallback", templateName);
		return m_FallbackMaterial;
	}

	auto mat = CreateRef<Material>(m_Device, name, tmpl);

	for (const auto &[propName, prop] : tmpl->properties) {
		if (prop.m_Type == ParameterType::Texture2D) {
			auto fallback = GetFallbackTextureForType(propName);
			mat->SetFallbackTexture(propName, fallback);
			if (!std::holds_alternative<Ref<Texture2D>>(prop.m_Value) || !std::get<Ref<Texture2D>>(prop.m_Value)) {
				mat->SetParameter(propName, fallback);
			}
		}
	}
	mat->CreateMaterialUBO();

	mat->MarkDirty();
	m_Materials[name] = mat;
	return mat;
}

void MaterialLibrary::RegisterMaterial(const Ref<Material> &material) {
	if (!material) {
		AQUILA_LOG_WARNING("Attempted to register null material");
		return;
	}
	if (m_Materials.contains(material->name)) {
		AQUILA_LOG_WARNING("Material '{}' already exists in library, overwriting", material->name);
	}

	m_Materials[material->name] = material;
	AQUILA_LOG_DEBUG("Registered material: {}", material->name);
}

bool MaterialLibrary::HasMaterial(const std::string &name) const {
	return m_Materials.contains(name);
}

Ref<Material> MaterialLibrary::GetMaterial(const std::string &name) {
	const auto it = m_Materials.find(name);
	return it != m_Materials.end() ? it->second : m_DefaultMaterial;
}

void MaterialLibrary::RegisterTemplate(const Ref<MaterialTemplate> &tmpl) {
	m_Templates[tmpl->name] = tmpl;
}

Ref<MaterialTemplate> MaterialLibrary::GetTemplate(const std::string &name) {
	const auto it = m_Templates.find(name);
	return it != m_Templates.end() ? it->second : nullptr;
}

void MaterialLibrary::CreateFallbackTextures() {
	m_WhiteTexture = Texture2D::Builder(m_Device, "Fallback_White").AsFallback(TextureType::Albedo).Build();
	m_NormalTexture = Texture2D::Builder(m_Device, "Fallback_Normal").AsFallback(TextureType::Normal).Build();
	m_BlackTexture = Texture2D::Builder(m_Device, "Fallback_Black").AsFallback(TextureType::Albedo).Build();
}

Ref<Texture2D> MaterialLibrary::GetFallbackTextureForType(const std::string &propName) {
	if (propName.find("normal") != std::string::npos || propName.find("Normal") != std::string::npos) {
		return m_NormalTexture;
	}
	return m_WhiteTexture;
}

void MaterialLibrary::CreateDefaultTemplates() {
	auto unlitTemplate = CreateRef<MaterialTemplate>("Unlit");
	unlitTemplate->shader = m_ShaderPrograms["Unlit"];

	unlitTemplate->AddProperty(MaterialParameter("color", ParameterType::Color, glm::vec4(1.0f)));

	auto mainTexParam = MaterialParameter("mainTexture", ParameterType::Texture2D, Ref<Texture2D>());
	mainTexParam.m_BindingIndex = 0;
	mainTexParam.m_DisplayName = "Texture";
	unlitTemplate->AddProperty(mainTexParam);
	unlitTemplate->renderState.m_DepthTest = true;
	RegisterTemplate(unlitTemplate);

	auto pbrTemplate = CreateRef<MaterialTemplate>("PBR");
	pbrTemplate->shader = m_ShaderPrograms["PBR"];

	pbrTemplate->AddProperty(MaterialParameter("albedoColor", ParameterType::Color, glm::vec4(1.0f)));
	pbrTemplate->AddProperty(MaterialParameter("metallic", ParameterType::Float, 0.0f));
	pbrTemplate->AddProperty(MaterialParameter("roughness", ParameterType::Float, 0.5f));
	pbrTemplate->AddProperty(MaterialParameter("emissiveColor", ParameterType::Color, glm::vec4(0.0f)));

	auto addTex = [&](const std::string &name, const std::string &display, int binding) {
		auto p = MaterialParameter(name, ParameterType::Texture2D, Ref<Texture2D>());
		p.m_BindingIndex = binding;
		p.m_DisplayName = display;
		pbrTemplate->AddProperty(p);
	};
	addTex("albedoMap", "Albedo Map", 0);
	addTex("normalMap", "Normal Map", 1);
	addTex("aoMap", "AO Map", 2);
	addTex("emissiveMap", "Emissive Map", 3);

	pbrTemplate->properties["albedoColor"].m_DisplayName = "Albedo Color";
	pbrTemplate->properties["metallic"].m_DisplayName = "Metallic";
	pbrTemplate->properties["metallic"].m_MinValue = 0.0F;
	pbrTemplate->properties["metallic"].m_MaxValue = 1.0F;
	pbrTemplate->properties["roughness"].m_DisplayName = "Roughness";
	pbrTemplate->properties["roughness"].m_MinValue = 0.0F;
	pbrTemplate->properties["roughness"].m_MaxValue = 1.0F;
	pbrTemplate->properties["emissiveColor"].m_DisplayName = "Emissive Color";
	RegisterTemplate(pbrTemplate);

	auto transparentTemplate = CreateRef<MaterialTemplate>("Transparent");
	transparentTemplate->shader = m_ShaderPrograms.contains("Transparent") ? m_ShaderPrograms["Transparent"] : nullptr;

	transparentTemplate->AddProperty(
		MaterialParameter("color", ParameterType::Color, glm::vec4(1.0f, 1.0f, 1.0f, 0.5f)));

	auto transTexParam = MaterialParameter("mainTexture", ParameterType::Texture2D, Ref<Texture2D>());
	transTexParam.m_DisplayName = "Texture";
	transparentTemplate->AddProperty(transTexParam);

	transparentTemplate->renderState.m_BlendMode = BlendMode::AlphaBlend;
	transparentTemplate->renderState.m_DepthWrite = false;
	RegisterTemplate(transparentTemplate);
}

void MaterialLibrary::CreateDefaultMaterials() {
	m_FallbackMaterial = CreateMaterial("FallbackMaterial", "Unlit");
	m_FallbackMaterial->SetParameter("color", glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
	m_FallbackMaterial->SetParameter("mainTexture", m_WhiteTexture);
	m_FallbackMaterial->MarkDirty();

	AQUILA_LOG_DEBUG("FallbackMaterial mainTexture: {}",
					 m_FallbackMaterial->GetTexture("mainTexture") ? "SET" : "NULL");

	m_DefaultMaterial = CreateMaterial("DefaultMaterial", "PBR");
	m_DefaultMaterial->SetParameter("albedoColor", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
	m_DefaultMaterial->SetParameter("emissiveColor", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	m_DefaultMaterial->SetParameter("metallic", 0.0f);
	m_DefaultMaterial->SetParameter("roughness", 0.5f);
	m_DefaultMaterial->SetParameter("albedoMap", m_WhiteTexture);
	m_DefaultMaterial->SetParameter("normalMap", m_NormalTexture);
	m_DefaultMaterial->SetParameter("aoMap", m_WhiteTexture);
	m_DefaultMaterial->SetParameter("emissiveMap", m_BlackTexture);
	m_DefaultMaterial->MarkDirty();

	AQUILA_LOG_DEBUG("DefaultMaterial albedoMap: {}", m_DefaultMaterial->GetTexture("albedoMap") ? "SET" : "NULL");
}

} // namespace Aquila::Graphics::Material
