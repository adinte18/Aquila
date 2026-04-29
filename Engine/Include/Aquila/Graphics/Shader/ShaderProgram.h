#ifndef AQUILA_SHADER_PROGRAM_H
#define AQUILA_SHADER_PROGRAM_H

#include "Aquila/Graphics/Material/MaterialParameters.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxDescriptorSet.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include "Aquila/RHI/ShaderCompiler.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Graphics::Shader {

// Internal — lives in .cpp only
struct ShaderStage {
	RHI::ShaderStageFlags stage;
	std::vector<uint32> spirv;
	std::string entryPointName;
	Slang::ComPtr<slang::IComponentType> linkedComponent;
	Slang::ComPtr<slang::ISession> session;
};

class ShaderProgram {
  public:
	enum class ShaderType : uint8 { Standard, PostProcess, Compute, Custom };

	enum class ReflectedBindingType : uint8 { CombinedImageSampler, UniformBuffer, StorageBuffer, Unknown };

	struct ReflectedBinding {
		std::string name;
		uint32 set;
		uint32 bindingIndex;
		uint32 descriptorCount;
		ReflectedBindingType type;
		RHI::ShaderStageFlags stageFlags;

		struct UBOField {
			std::string name;
			Graphics::ParameterType paramType;
		};
		std::vector<UBOField> uboFields;
	};

	// Public members — engine types only
	std::string m_Name;
	Ref<GFX::GfxDescriptorSetLayout> m_DescriptorSetLayout;

	ShaderProgram(GFX::GfxContext &ctx, std::string name) : m_Name(std::move(name)), m_Context(ctx) {}
	~ShaderProgram() { Cleanup(); }

	AQUILA_NONCOPYABLE(ShaderProgram);

	bool AddStageFromSlang(const std::string &slangPath, std::string &errorLog);
	bool Reload(std::string &errorLog, Ref<GFX::GfxDescriptorSetLayout> &outNewLayout);
	void CommitNewLayout(Ref<GFX::GfxDescriptorSetLayout> newLayout);
	bool Reflect();
	void Cleanup();

	void SetTargetFormats(std::vector<RHI::TextureFormat> colorFormats,
						  RHI::TextureFormat depthFormat = RHI::TextureFormat::Depth32) {
		m_ColorFormats = std::move(colorFormats);
		m_DepthFormat = depthFormat;
	}

	[[nodiscard]] const std::vector<RHI::TextureFormat> &GetColorFormats() const { return m_ColorFormats; }
	[[nodiscard]] RHI::TextureFormat GetDepthFormat() const { return m_DepthFormat; }
	[[nodiscard]] bool IsValid() const { return !m_Stages.empty(); }
	[[nodiscard]] const std::string &GetSlangPath() const { return m_SlangPath; }
	[[nodiscard]] ShaderType GetShaderType() const { return m_ShaderType; }
	[[nodiscard]] const std::vector<ReflectedBinding> &GetReflectedBindings() const { return m_ReflectedBindings; }
	void SetShaderType(ShaderType type) { m_ShaderType = type; }

	// Returns SPIRV stage desc for pipeline creation via GfxContext
	[[nodiscard]] RHI::ShaderStageDesc GetStageDesc(RHI::ShaderStageFlags stage) const;

  private:
	GFX::GfxContext &m_Context;
	std::string m_SlangPath;
	ShaderType m_ShaderType = ShaderType::Standard;
	std::vector<RHI::TextureFormat> m_ColorFormats;
	RHI::TextureFormat m_DepthFormat = RHI::TextureFormat::Depth32;
	std::vector<ReflectedBinding> m_ReflectedBindings;
	std::vector<ShaderStage> m_Stages;

	struct BindingInfo {
		RHI::DescriptorType descriptorType = RHI::DescriptorType::UniformBuffer;
		RHI::ShaderStageFlags stageFlags = RHI::ShaderStageFlags::None;
		uint32 descriptorCount = 1;
		bool occupied = false;
	};

	static bool SlangTypeToDescriptor(slang::TypeLayoutReflection *typeLayout, RHI::DescriptorType &outType,
									  ReflectedBindingType &outReflectedType);

	void ProcessBinding(slang::VariableLayoutReflection *var, RHI::ShaderStageFlags stageFlags,
						std::map<uint32, std::map<uint32, BindingInfo>> &sets);

	bool ReflectInto(Ref<GFX::GfxDescriptorSetLayout> &outLayout);
};

} // namespace Aquila::Graphics::Shader
#endif
