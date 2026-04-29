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

class Material {
  public:
	std::string name;

	// Create from a reflected ShaderProgram.
	// The shader must have Reflect() called already.
	// Parameter list and UBO layout are derived from shader reflection.
	// The caller supplies the pre-built pipeline (it owns vertex layout / render target formats).
	static Ref<Material> CreateFromShader(GFX::GfxContext &ctx, Shader::ShaderProgram &shader,
										  Ref<GFX::GfxPipeline> pipeline);

	// Manual mode: caller supplies the fully-built pipeline and optional descriptor layout.
	// Call RegisterParameter() after creation to describe UBO fields.
	// If no layout is supplied, only the pipeline is bound.
	static Ref<Material> Create(GFX::GfxContext &ctx, Ref<GFX::GfxPipeline> pipeline,
								Ref<GFX::GfxDescriptorSetLayout> layout = nullptr);

	// Register a named parameter. Used internally by CreateFromShader;
	// also callable by hand in manual-mode materials.
	void RegisterParameter(std::string paramName, ParameterType type, uint32 uboOffset,
						   uint32 textureBinding = UINT32_MAX);

	// Named parameter setters — find param by name and patch the UBO blob.
	Material &Set(const std::string &paramName, f32 v);
	Material &Set(const std::string &paramName, int v);
	Material &Set(const std::string &paramName, bool v);
	Material &Set(const std::string &paramName, const vec2 &v);
	Material &Set(const std::string &paramName, const vec3 &v);
	Material &Set(const std::string &paramName, const vec4 &v);
	Material &Set(const std::string &paramName, Ref<GFX::GfxTexture> tex);

	// Convenience PBR setters (fall through to named Set).
	Material &SetAlbedo(const vec4 &color)	{ return Set("albedo", color); }
	Material &SetAlbedo(const vec3 &color)	{ return Set("albedo", vec4{color, 1.f}); }
	Material &SetMetallic(f32 v)			{ return Set("metallic", v); }
	Material &SetRoughness(f32 v)			{ return Set("roughness", v); }
	Material &SetEmissive(f32 v)			{ return Set("emissive", v); }

	// Bind a texture directly by descriptor-set binding index
	// (for textures not registered as named parameters).
	Material &SetTexture(uint32 binding, GFX::GfxTexture &tex);

	// Upload dirty CPU state to GPU. Call once per frame before Bind().
	void Flush();

	// Bind the pipeline, then (if present) the descriptor set at setIndex.
	void Bind(GFX::GfxCommandList &cmd, uint32 setIndex = 1);

	[[nodiscard]] GFX::GfxPipeline				   &GetPipeline()		{ return *m_Pipeline; }
	[[nodiscard]] bool								HasDescriptorSet() const { return m_Set != nullptr; }
	[[nodiscard]] const std::vector<MaterialParameter> &GetParameters() const { return m_Parameters; }
	[[nodiscard]] const MaterialParameter			   *GetParameter(const std::string &paramName) const;

  private:
	Material() = default;

	GFX::GfxContext					*m_Context = nullptr;
	Ref<GFX::GfxPipeline>			 m_Pipeline;
	Ref<GFX::GfxDescriptorSetLayout> m_Layout;
	Ref<GFX::GfxDescriptorSet>		 m_Set;
	Ref<GFX::GfxBuffer>				 m_UniformBuffer;

	std::vector<MaterialParameter>	 m_Parameters;
	std::vector<uint8>				 m_UBOData;		// raw CPU-side UBO blob
	bool							 m_UniformDirty = false;

	struct PendingTexture {
		uint32			  binding;
		GFX::GfxTexture *tex;
	};
	std::vector<PendingTexture> m_PendingTextures;

	MaterialParameter *FindParameter(const std::string &paramName);

	template <typename T>
	void WriteUBO(uint32 offset, const T &v);

	// Lazily creates the UBO and binds it to the descriptor set.
	// Called by Flush() the first time the material has UBO data.
	void EnsureUniformBuffer();
};

} // namespace Aquila::Graphics
