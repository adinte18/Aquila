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
	std::vector<Uint32> spirv;
	std::string entry_point_name;
	Slang::ComPtr<slang::IComponentType> linked_component;
	Slang::ComPtr<slang::ISession> session;
};

class ShaderProgram {
  public:
	enum class ShaderType : Uint8 { Standard, PostProcess, Compute, Custom };

	enum class ReflectedBindingType : Uint8 { CombinedImageSampler, UniformBuffer, StorageBuffer, Unknown };

	struct ReflectedBinding {
		std::string name;
		Uint32 set;
		Uint32 binding_index;
		Uint32 descriptor_count;
		ReflectedBindingType type;
		RHI::ShaderStageFlags stage_flags;

		struct UBOField {
			std::string name;
			Graphics::ParameterType param_type;
		};
		std::vector<UBOField> ubo_fields;
	};

	// Public members — engine types only
	std::string m_name;
	Ref<GFX::GfxDescriptorSetLayout> m_descriptor_set_layout;

	ShaderProgram(GFX::GfxContext &ctx, std::string name) : m_name(std::move(name)), m_context(ctx) {}
	~ShaderProgram() { cleanup(); }

	AQUILA_NONCOPYABLE(ShaderProgram);

	bool add_stage_from_slang(const std::string &slang_path, std::string &error_log);
	bool reload(std::string &error_log, Ref<GFX::GfxDescriptorSetLayout> &out_new_layout);
	void commit_new_layout(Ref<GFX::GfxDescriptorSetLayout> new_layout);
	bool reflect();
	void cleanup();

	void set_target_formats(std::vector<RHI::TextureFormat> color_formats,
						  RHI::TextureFormat depth_format = RHI::TextureFormat::Depth32) {
		m_color_formats = std::move(color_formats);
		m_depth_format = depth_format;
	}

	[[nodiscard]] const std::vector<RHI::TextureFormat> &get_color_formats() const { return m_color_formats; }
	[[nodiscard]] RHI::TextureFormat get_depth_format() const { return m_depth_format; }
	[[nodiscard]] bool is_valid() const { return !m_stages.empty(); }
	[[nodiscard]] const std::string &get_slang_path() const { return m_slang_path; }
	[[nodiscard]] ShaderType get_shader_type() const { return m_shader_type; }
	[[nodiscard]] const std::vector<ReflectedBinding> &get_reflected_bindings() const { return m_reflected_bindings; }
	void set_shader_type(ShaderType type) { m_shader_type = type; }

	// Returns SPIRV stage desc for pipeline creation via GfxContext
	[[nodiscard]] RHI::ShaderStageDesc get_stage_desc(RHI::ShaderStageFlags stage) const;

  private:
	GFX::GfxContext &m_context;
	std::string m_slang_path;
	ShaderType m_shader_type = ShaderType::Standard;
	std::vector<RHI::TextureFormat> m_color_formats;
	RHI::TextureFormat m_depth_format = RHI::TextureFormat::Depth32;
	std::vector<ReflectedBinding> m_reflected_bindings;
	std::vector<ShaderStage> m_stages;

	struct BindingInfo {
		RHI::DescriptorType descriptor_type = RHI::DescriptorType::UniformBuffer;
		RHI::ShaderStageFlags stage_flags = RHI::ShaderStageFlags::None;
		Uint32 descriptor_count = 1;
		bool occupied = false;
	};

	static bool slang_type_to_descriptor(slang::TypeLayoutReflection *type_layout, RHI::DescriptorType &out_type,
									  ReflectedBindingType &out_reflected_type);

	void process_binding(slang::VariableLayoutReflection *var, RHI::ShaderStageFlags stage_flags,
						std::map<Uint32, std::map<Uint32, BindingInfo>> &sets);

	bool reflect_into(Ref<GFX::GfxDescriptorSetLayout> &out_layout);
};

} // namespace Aquila::Graphics::Shader
#endif
