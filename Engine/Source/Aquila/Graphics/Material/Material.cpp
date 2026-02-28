#include "Aquila/Graphics/Material/Material.h"

namespace Aquila::Graphics::Material {

void Material::SetShaderAsset(const std::string &shaderAssetPath) {
	m_ShaderAssetPath = shaderAssetPath;
	MarkPipelineDirty();
}

void Material::SetShader(const Ref<Shader::ShaderProgram> &shader) {
	if (!m_Template) {
		return;
	}

	m_Template->shader = shader;

	if (shader) {
		m_Template->shaderType = shader->GetShaderType();
	}

	SyncPropertiesFromShader();
	MarkPipelineDirty();
}

Ref<Shader::ShaderProgram> Material::GetShader() const {
	return m_Template ? m_Template->shader : nullptr;
}

void Material::SyncPropertiesFromShader() {
	if (!m_Template || !m_Template->shader) {
		return;
	}

	std::unordered_set<std::string> reflectedNames;
	for (const auto &binding : m_Template->shader->GetReflectedBindings()) {
		if (binding.set != 1) {
			continue;
		}
		switch (binding.type) {
		case Shader::ShaderProgram::ReflectedBindingType::CombinedImageSampler:
			reflectedNames.insert(binding.name);
			break;
		case Shader::ShaderProgram::ReflectedBindingType::UniformBuffer:
			for (const auto &field : binding.uboFields) {
				reflectedNames.insert(field.name);
			}
			break;
		default:
			break;
		}
	}

	std::vector<std::string> toRemove;
	for (const auto &[propName, prop] : m_Template->properties) {
		if (!reflectedNames.contains(propName)) {
			toRemove.push_back(propName);
		}
	}
	for (const auto &propName : toRemove) {
		m_Template->properties.erase(propName);
		m_Overrides.erase(propName); // remove stale overrides
	}

	for (const auto &binding : m_Template->shader->GetReflectedBindings()) {
		if (binding.set != 1) {
			continue;
		}
		if (m_Template->HasProperty(binding.name)) {
			continue;
		}

		switch (binding.type) {
		case Shader::ShaderProgram::ReflectedBindingType::CombinedImageSampler: {
			MaterialParameter param(binding.name, ParameterType::Texture2D, Ref<Resources::Texture2D>());
			param.m_BindingIndex = binding.bindingIndex;
			param.m_DisplayName = binding.name;
			m_Template->AddProperty(param);
			AQUILA_LOG_DEBUG("Material '{}': registered texture slot '{}' (binding {})", name, binding.name,
							 binding.bindingIndex);
			break;
		}
		case Shader::ShaderProgram::ReflectedBindingType::UniformBuffer: {
			if (binding.set == 0) {
				break;
			}

			for (const auto &field : binding.uboFields) {
				if (m_Template->HasProperty(field.name)) {
					continue;
				}

				ParameterValue defaultVal;
				switch (field.paramType) {
				case ParameterType::Color:
					defaultVal = glm::vec4(1.f, 1.f, 1.f, 1.f);
					break;
				case ParameterType::Float:
					defaultVal = 0.f;
					break;
				case ParameterType::Int:
					defaultVal = 0;
					break;
				case ParameterType::Vec2:
					defaultVal = glm::vec2(0.f);
					break;
				case ParameterType::Vec3:
					defaultVal = glm::vec4(0.f);
					break;
				default:
					continue;
				}

				MaterialParameter param(field.name, field.paramType, defaultVal);
				param.m_DisplayName = field.name;
				m_Template->AddProperty(param);

				AQUILA_LOG_DEBUG("Material '{}': registered UBO field '{}' from cbuffer '{}'", name, field.name,
								 binding.name);
			}
			break;
		}
		default:
			break;
		}
	}
}

// ── UBO ──────────────────────────────────────────────────────────────────────

void Material::DestroyMaterialUBO() {
	for (auto &[binding, buffer] : m_UBOBuffers) {
		if (buffer) {
			buffer->UnMap();
		}
	}
	m_UBOBuffers.clear();
}

void Material::CreateMaterialUBO() {
	if (!m_Template || !m_Template->shader) {
		return;
	}

	for (const auto &binding : m_Template->shader->GetReflectedBindings()) {
		if (binding.type != Shader::ShaderProgram::ReflectedBindingType::UniformBuffer) {
			continue;
		}
		if (binding.set == 0) {
			continue;
		}
		if (m_UBOBuffers.contains(binding.bindingIndex)) {
			continue;
		}

		size_t uboSize = ComputeUBOSizeForBinding(binding);
		if (uboSize == 0) {
			AQUILA_LOG_WARNING("Material '{}': UBO '{}' at binding {} has no fields", name, binding.name,
							   binding.bindingIndex);
			continue;
		}

		auto buffer =
			CreateUnique<Resources::Buffer>(m_Device, name + "_UBO_" + std::to_string(binding.bindingIndex), uboSize, 1,
											VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
											VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		buffer->Map();

		if (buffer->GetBuffer() == VK_NULL_HANDLE) {
			AQUILA_LOG_ERROR("Material '{}': UBO buffer creation failed for binding {}", name, binding.bindingIndex);
			continue;
		}

		m_UBOBuffers[binding.bindingIndex] = std::move(buffer);
		AQUILA_LOG_DEBUG("Material '{}': created UBO '{}' at binding {} ({} bytes)", name, binding.name,
						 binding.bindingIndex, uboSize);
	}

	UpdateMaterialUBO();
}

size_t Material::ComputeUBOSizeForBinding(const Shader::ShaderProgram::ReflectedBinding &binding) const {
	size_t size = 0;
	for (const auto &field : binding.uboFields) {
		switch (field.paramType) {
		case ParameterType::Color:
			size += sizeof(glm::vec4);
			break;
		case ParameterType::Float:
			size += sizeof(float);
			break;
		case ParameterType::Int:
			size += sizeof(int32_t);
			break;
		case ParameterType::Vec2:
			size += sizeof(glm::vec2);
			break;
		case ParameterType::Vec3:
			size += sizeof(glm::vec4);
			break;
		default:
			break;
		}
	}
	return (size + 15) & ~size_t(15);
}

void Material::UpdateMaterialUBO() {
	if (m_UBOBuffers.empty() || !m_Template || !m_Template->shader) {
		return;
	}

	for (const auto &binding : m_Template->shader->GetReflectedBindings()) {
		if (binding.type != Shader::ShaderProgram::ReflectedBindingType::UniformBuffer) {
			continue;
		}
		if (binding.set == 0) {
			continue;
		}

		auto it = m_UBOBuffers.find(binding.bindingIndex);
		if (it == m_UBOBuffers.end()) {
			continue;
		}

		auto &buffer = it->second;
		std::vector<uint8_t> staging(buffer->GetBufferSize(), 0);
		size_t offset = 0;

		for (const auto &field : binding.uboFields) {
			auto val = GetProperty(field.name);
			switch (field.paramType) {
			case ParameterType::Color: {
				if (offset + sizeof(glm::vec4) > staging.size()) {
					break;
				}
				if (auto *v = std::get_if<glm::vec4>(&val)) {
					memcpy(staging.data() + offset, v, sizeof(glm::vec4));
				}
				offset += sizeof(glm::vec4);
				break;
			}
			case ParameterType::Float: {
				if (offset + sizeof(float) > staging.size()) {
					break;
				}
				if (auto *v = std::get_if<float>(&val)) {
					memcpy(staging.data() + offset, v, sizeof(float));
				}
				offset += sizeof(float);
				break;
			}
			case ParameterType::Int: {
				if (offset + sizeof(int32_t) > staging.size()) {
					break;
				}
				if (auto *v = std::get_if<int32_t>(&val)) {
					memcpy(staging.data() + offset, v, sizeof(int32_t));
				}
				offset += sizeof(int32_t);
				break;
			}
			case ParameterType::Vec2: {
				if (offset + sizeof(glm::vec2) > staging.size()) {
					break;
				}
				if (auto *v = std::get_if<glm::vec2>(&val)) {
					memcpy(staging.data() + offset, v, sizeof(glm::vec2));
				}
				offset += sizeof(glm::vec2);
				break;
			}
			case ParameterType::Vec3: {
				if (offset + sizeof(glm::vec4) > staging.size()) {
					break;
				}
				if (auto *v = std::get_if<glm::vec4>(&val)) {
					memcpy(staging.data() + offset, v, sizeof(glm::vec4));
				}
				offset += sizeof(glm::vec4);
				break;
			}
			default:
				break;
			}
		}

		buffer->Write(staging.data(), staging.size());
		AQUILA_VULKAN_CHECK(buffer->Flush());
	}
} // ── Parameters ───────────────────────────────────────────────────────────────

void Material::SetParameter(const std::string &name, const ParameterValue &value) {
	if (!m_Template || !m_Template->HasProperty(name)) {
		return;
	}

	if (!m_Overrides.contains(name)) {
		m_Overrides[name] = *m_Template->GetProperty(name);
	}

	auto &prop = m_Overrides[name];

	if (prop.m_Type == ParameterType::Texture2D) {
		auto tex = std::get<Ref<Resources::Texture2D>>(value);
		if (!tex) {
			auto iterator = m_FallbackTextures.find(name);
			if (iterator != m_FallbackTextures.end() && iterator->second) {
				tex = iterator->second;
			}
		}
		prop.m_Value = tex;
		MarkDescriptorDirty();
	} else {
		prop.m_Value = value;
		UpdateMaterialUBO();
		m_IsDirty = true;
	}
}

ParameterValue Material::GetProperty(const std::string &name) const {
	if (auto iterator = m_Overrides.find(name); iterator != m_Overrides.end()) {
		return iterator->second.m_Value;
	}
	const auto *property = m_Template->GetProperty(name);
	return (property != nullptr) ? property->m_Value : ParameterValue{};
}

Ref<Resources::Texture2D> Material::GetTexture(const std::string &name) const {
	if (auto val = GetProperty(name); std::holds_alternative<Ref<Resources::Texture2D>>(val)) {
		if (auto tex = std::get<Ref<Resources::Texture2D>>(val)) {
			return tex;
		}
	}
	auto iterator = m_FallbackTextures.find(name);
	return (iterator != m_FallbackTextures.end() && iterator->second) ? iterator->second : nullptr;
}

void Material::SetFallbackTexture(const std::string &name, Ref<Resources::Texture2D> texture) {
	m_FallbackTextures[name] = std::move(texture);
}

bool Material::IsFallbackTexture(const std::string &name, const Ref<Resources::Texture2D> &texture) const {
	auto iterator = m_FallbackTextures.find(name);
	return iterator != m_FallbackTextures.end() && iterator->second == texture;
}

void Material::ReSetParameter(const std::string &name) {
	m_Overrides.erase(name);
	m_IsDirty = true;
}

void Material::ResetAllProperties() {
	m_Overrides.clear();
	m_IsDirty = true;
}

// Template / render state

void Material::SetTemplate(const Ref<MaterialTemplate> &tmpl) {
	m_Template = tmpl;
	m_RenderState = tmpl->renderState;
	if (tmpl->shader) {
		m_Template->shaderType = tmpl->shader->GetShaderType();
	}
	m_IsDirty = true;
}

Ref<MaterialTemplate> Material::GetTemplate() const {
	return m_Template;
}
void Material::SetRenderState(const RenderState &state) {
	m_RenderState = state;
	MarkPipelineDirty();
}
const RenderState &Material::GetRenderState() const {
	return m_RenderState;
}

// Dirty flags

bool Material::IsDirty() const {
	return m_IsDirty;
}
bool Material::IsPipelineDirty() const {
	return m_PipelineDirty;
}
bool Material::IsDescriptorDirty() const {
	return m_DescriptorDirty;
}

void Material::MarkClean() {
	m_IsDirty = m_PipelineDirty = m_DescriptorDirty = false;
}
void Material::MarkDirty() {
	m_IsDirty = true;
}
void Material::MarkPipelineDirty() {
	m_IsDirty = true;
	m_PipelineDirty = true;
}
void Material::MarkDescriptorDirty() {
	m_IsDirty = true;
	m_DescriptorDirty = true;
}
void Material::MarkPipelineClean() {
	m_PipelineDirty = false;
}
void Material::MarkDescriptorClean() {
	m_DescriptorDirty = false;
	if (!m_PipelineDirty) {
		m_IsDirty = false;
	}
}

//  Descriptor sets / pipeline

VkDescriptorSet Material::GetDescriptorSet(const uint32 &frameIndex) const {
	return m_DescriptorSets[frameIndex];
}
void Material::SetDescriptorSet(VkDescriptorSet set, const uint32 &frameIndex) {
	m_DescriptorSets[frameIndex] = set;
}
VkPipeline Material::GetPipeline() const {
	return m_Pipeline;
}
void Material::SetPipeline(VkPipeline pipeline) {
	m_Pipeline = pipeline;
}

std::unordered_map<std::string, MaterialParameter> Material::GetAllProperties() const {
	std::unordered_map<std::string, MaterialParameter> props;
	for (const auto &[n, prop] : m_Template->properties) {
		props.emplace(n, prop);
	}
	for (const auto &[n, prop] : m_Overrides) {
		if (auto iterator = props.find(n); iterator != props.end()) {
			iterator->second.m_Value = prop.m_Value;
		}
	}
	return props;
}

} // namespace Aquila::Graphics::Material
