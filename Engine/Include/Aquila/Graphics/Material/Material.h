#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/Material/MaterialParameters.h"
#include "Aquila/GFX/GfxPipeline.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/GFX/GfxBuffer.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/GFX/GfxCommandList.h"

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::Graphics::Shader {
class ShaderProgram;
}

namespace Aquila::Graphics {

enum class MaterialType : uint8 {
	PBR,
	Lit,
	Unlit,
	Custom,
};

class MaterialFactory;

class Material {
  public:
	std::string name;

	static Ref<Material> CreateFromShader(GFX::GfxContext &ctx, Shader::ShaderProgram &shader,
										  Ref<GFX::GfxPipeline> pipeline);

	static Ref<Material> Create(GFX::GfxContext &ctx, Ref<GFX::GfxPipeline> pipeline,
								Ref<GFX::GfxDescriptorSetLayout> layout = nullptr);

	void RegisterParameter(std::string paramName, ParameterType type, uint32 uboOffset,
						   uint32 textureBinding = UINT32_MAX);

	Material &Set(const std::string &paramName, f32 v);
	Material &Set(const std::string &paramName, int v);
	Material &Set(const std::string &paramName, bool v);
	Material &Set(const std::string &paramName, const vec2 &v);
	Material &Set(const std::string &paramName, const vec3 &v);
	Material &Set(const std::string &paramName, const vec4 &v);
	Material &Set(const std::string &paramName, Ref<GFX::GfxTexture> tex);

	Material &SetAlbedo(const vec4 &color) { return Set("albedo", color); }
	Material &SetAlbedo(const vec3 &color) { return Set("albedo", vec4{ color, 1.f }); }
	Material &SetMetallic(f32 v) { return Set("metallic", v); }
	Material &SetRoughness(f32 v) { return Set("roughness", v); }
	Material &SetEmissive(f32 v) { return Set("emissive", v); }

	Material &SetTexture(uint32 binding, GFX::GfxTexture &tex);

	void Flush();

	void Bind(GFX::GfxCommandList &cmd, uint32 setIndex = 1);

	[[nodiscard]] GFX::GfxPipeline &GetPipeline() { return *m_Pipeline; }
	[[nodiscard]] bool HasDescriptorSet() const { return m_Set != nullptr; }
	[[nodiscard]] const std::vector<MaterialParameter> &GetParameters() const { return m_Parameters; }
	[[nodiscard]] const MaterialParameter *GetParameter(const std::string &paramName) const;

	[[nodiscard]] MaterialType GetType() const { return m_Type; }
	void SetType(MaterialType type) { m_Type = type; }
	[[nodiscard]] const std::string &GetShaderPath() const { return m_ShaderPath; }

  private:
	Material() = default;

	void ReplacePipeline(Ref<GFX::GfxPipeline> newPipeline, Ref<GFX::GfxDescriptorSetLayout> newLayout = nullptr);

	friend class MaterialFactory;

	GFX::GfxContext *m_Context = nullptr;
	Ref<GFX::GfxPipeline> m_Pipeline;
	MaterialType m_Type = MaterialType::PBR;
	std::string m_ShaderPath;
	Ref<GFX::GfxDescriptorSetLayout> m_Layout;
	Ref<GFX::GfxDescriptorSet> m_Set;
	Ref<GFX::GfxBuffer> m_UniformBuffer;

	std::vector<MaterialParameter> m_Parameters;
	std::vector<uint8> m_UBOData;
	bool m_UniformDirty = false;

	struct PendingTexture {
		uint32 binding;
		GFX::GfxTexture *tex;
	};
	std::vector<PendingTexture> m_PendingTextures;

	MaterialParameter *FindParameter(const std::string &paramName);

	template <typename T> void WriteUBO(uint32 offset, const T &v);

	void EnsureUniformBuffer();
};

} // namespace Aquila::Graphics
