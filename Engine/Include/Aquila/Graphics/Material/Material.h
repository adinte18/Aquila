#ifndef AQUILA_MATERIAL_H
#define AQUILA_MATERIAL_H

#include <utility>
#include "Aquila/Graphics/Material/MaterialParameters.h"
#include "Aquila/Graphics/Resources/Buffer.h"
#include "Aquila/Graphics/Shader/ShaderProgram.h"

namespace Aquila::Graphics::Material {

enum class BlendMode : std::uint8_t { Opaque, AlphaBlend, Additive, Multiply };
enum class CullMode : std::uint8_t { None, Front, Back };

using ShaderType = Shader::ShaderProgram::ShaderType;

struct RenderState {
	BlendMode m_BlendMode = BlendMode::Opaque;
	CullMode m_CullMode = CullMode::Back;
	bool m_DepthTest = true;
	bool m_DepthWrite = true;
	bool m_Wireframe = false;
	f32 m_LineWidth = 1.0F;
};

struct MaterialTemplate {
	std::string name;
	Ref<Shader::ShaderProgram> shader;
	RenderState renderState;
	ShaderType shaderType = ShaderType::Standard;
	std::unordered_map<std::string, MaterialParameter> properties;

	MaterialTemplate(std::string n) : name(std::move(n)) {}
	std::vector<std::string> propertyOrder;
	void AddProperty(const MaterialParameter &prop) {
		if (!properties.contains(prop.m_Name)) {
			propertyOrder.push_back(prop.m_Name); // track order
		}
		properties[prop.m_Name] = prop;
	}
	bool HasProperty(const std::string &n) const { return properties.contains(n); }
	const MaterialParameter *GetProperty(const std::string &n) const {
		auto it = properties.find(n);
		return it != properties.end() ? &it->second : nullptr;
	}
};

class Material {
  public:
	std::string name;

	Material(Device &device, std::string n, const Ref<MaterialTemplate> &tmpl)
		: name(std::move(n)), m_Device(device), m_Template(tmpl) {
		if (tmpl && tmpl->shader) {
			m_Template->shaderType = tmpl->shader->GetShaderType();
		}
	}

	void SetShaderAsset(const std::string &shaderAssetPath);
	void SetShader(const Ref<Shader::ShaderProgram> &shader);
	const std::string &GetShaderAssetPath() const { return m_ShaderAssetPath; }
	Ref<Shader::ShaderProgram> GetShader() const;
	bool HasShader() const { return m_Template && m_Template->shader != nullptr; }

	void SyncPropertiesFromShader();

	void SetParameter(const std::string &name, const ParameterValue &value);

	ParameterValue GetProperty(const std::string &name) const;
	Ref<Resources::Texture2D> GetTexture(const std::string &name) const;
	bool IsFallbackTexture(const std::string &name, const Ref<Resources::Texture2D> &texture) const;
	void SetFallbackTexture(const std::string &name, Ref<Resources::Texture2D> texture);

	void ReSetParameter(const std::string &name);
	void ResetAllProperties();

	void SetRenderState(const RenderState &state);
	const RenderState &GetRenderState() const;

	Ref<MaterialTemplate> GetTemplate() const;
	void SetTemplate(const Ref<MaterialTemplate> &tmpl);

	ShaderType GetShaderType() const { return m_Template ? m_Template->shaderType : ShaderType::Standard; }
	bool IsCustomShader() const { return GetShaderType() != ShaderType::Standard; }

	bool IsDirty() const;
	bool IsPipelineDirty() const;
	bool IsDescriptorDirty() const;
	void MarkClean();
	void MarkDescriptorDirty();
	void MarkPipelineDirty();
	void MarkPipelineClean();
	void MarkDescriptorClean();
	void MarkDirty();

	VkDescriptorSet GetDescriptorSet(const uint32 &frameIndex) const;
	void SetDescriptorSet(VkDescriptorSet set, const uint32 &frameIndex);
	VkPipeline GetPipeline() const;
	void SetPipeline(VkPipeline pipeline);

	std::unordered_map<std::string, MaterialParameter> GetAllProperties() const;

	Resources::Buffer *GetUBOBuffer(uint32 bindingIndex) const {
		auto it = m_UBOBuffers.find(bindingIndex);
		return it != m_UBOBuffers.end() ? it->second.get() : nullptr;
	}
	bool HasAnyUBO() const { return !m_UBOBuffers.empty(); }
	void DestroyMaterialUBO();
	void CreateMaterialUBO();
	void UpdateMaterialUBO();
	size_t ComputeUBOSizeForBinding(const Shader::ShaderProgram::ReflectedBinding &binding) const;

	void MarkAsEngineMaterial() { m_IsEngineMaterial = true; }
	bool IsEngineMaterial() const { return m_IsEngineMaterial; }

  private:
	struct PBRMaterialUBO {
		alignas(16) glm::vec4 albedoColor;
		alignas(16) glm::vec4 emissiveColor;
		alignas(4) f32 metallic;
		alignas(4) f32 roughness;
		alignas(8) glm::vec2 padding;
	};

	struct UnlitMaterialUBO {
		alignas(16) glm::vec4 color;
	};

	Device &m_Device;
	Ref<MaterialTemplate> m_Template;
	std::string m_ShaderAssetPath;

	std::unordered_map<std::string, MaterialParameter> m_Overrides;
	std::unordered_map<std::string, Ref<Resources::Texture2D>> m_FallbackTextures;

	RenderState m_RenderState;

	bool m_IsDirty = false;
	bool m_PipelineDirty = true;
	bool m_DescriptorDirty = true;
	bool m_IsEngineMaterial = false;

	std::array<VkDescriptorSet, 3> m_DescriptorSets = { VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE };
	VkPipeline m_Pipeline = VK_NULL_HANDLE;

	std::unordered_map<uint32, Unique<Resources::Buffer>> m_UBOBuffers;
};

} // namespace Aquila::Graphics::Material
#endif
