#ifndef AQUILA_SHADER_PROGRAM_H
#define AQUILA_SHADER_PROGRAM_H

#include "Aquila/Graphics/Material/MaterialParameters.h"
#include "Aquila/Graphics/Pipeline/DynamicRenderingHelper.h"
#include "Aquila/Graphics/Shader/Shader.h"
#include "Aquila/Graphics/Shader/ShaderCompiler.h"

namespace Aquila::Graphics::Shader {

struct ShaderStage {
	VkShaderStageFlagBits stage;
	std::vector<uint32> spirv;
	std::string entryPointName;
	Slang::ComPtr<slang::IComponentType> linkedComponent;
	Slang::ComPtr<slang::ISession> session;
	VkShaderModule module = VK_NULL_HANDLE;
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
		VkShaderStageFlags stageFlags;

		struct UBOField {
			std::string name;
			Material::ParameterType paramType;
		};
		std::vector<UBOField> uboFields;
	};

	std::string m_Name;
	std::vector<ShaderStage> m_Stages;
	VkPipelineLayout m_Layout = VK_NULL_HANDLE;
	Ref<RenderingPipeline::DescriptorSetLayout> m_DescriptorSetLayout;

	ShaderProgram(const ShaderProgram &) = default;
	ShaderProgram(ShaderProgram &&) = delete;
	ShaderProgram &operator=(const ShaderProgram &) = delete;
	ShaderProgram &operator=(ShaderProgram &&) = delete;
	ShaderProgram(Device &device, std::string name) : m_Name(std::move(name)), m_Device(device) {}
	~ShaderProgram() { Cleanup(); }

	bool AddStageFromSlang(const std::string &slangPath, std::string &errorLog) {
		std::vector<CompiledStage> compiled;
		if (!ShaderCompiler::CompileFile(slangPath, compiled, errorLog)) {
			AQUILA_LOG_ERROR("Shader '{}': compilation failed for '{}': {}", m_Name, slangPath, errorLog);
			return false;
		}

		for (auto &compiledShader : compiled) {
			VkShaderModule mod = CreateShaderModule(compiledShader.spirv);
			m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64>(mod),
										(m_Name + "_" + compiledShader.entryPointName).c_str());
			m_Stages.push_back({ compiledShader.stage, std::move(compiledShader.spirv), compiledShader.entryPointName,
								 compiledShader.linkedComponent, compiledShader.session, mod });
		}

		if (!m_Stages.empty() && m_Stages[0].stage == VK_SHADER_STAGE_COMPUTE_BIT) {
			m_ShaderType = ShaderType::Compute;
		}

		m_SlangPath = slangPath;
		AQUILA_LOG_INFO("Shader '{}': loaded {} stage(s) from '{}'", m_Name, compiled.size(), slangPath);
		return true;
	}

	bool Reload(std::string &errorLog, Ref<RenderingPipeline::DescriptorSetLayout> &outNewLayout) {
		if (m_SlangPath.empty()) {
			errorLog = "No Slang source path recorded for '" + m_Name + "'";
			return false;
		}
		AQUILA_LOG_INFO("Shader '{}': reloading '{}'", m_Name, m_SlangPath);

		std::vector<CompiledStage> compiled;
		if (!ShaderCompiler::CompileFile(m_SlangPath, compiled, errorLog)) {
			AQUILA_LOG_ERROR("Shader '{}': reload compile failed: {}", m_Name, errorLog);
			return false;
		}

		for (auto &stage : m_Stages) {
			if (stage.module != VK_NULL_HANDLE) {
				vkDestroyShaderModule(m_Device.GetDevice(), stage.module, nullptr);
				stage.module = VK_NULL_HANDLE;
			}
		}
		m_Stages.clear();

		for (auto &compiledStage : compiled) {
			VkShaderModule mod = CreateShaderModule(compiledStage.spirv);
			m_Device.SetObjectDebugName(VK_OBJECT_TYPE_SHADER_MODULE, reinterpret_cast<uint64>(mod),
										(m_Name + "_" + compiledStage.entryPointName).c_str());
			m_Stages.push_back({ compiledStage.stage, std::move(compiledStage.spirv), compiledStage.entryPointName,
								 compiledStage.linkedComponent, compiledStage.session, mod });
		}

		AQUILA_LOG_INFO("Shader '{}': reload succeeded, re-reflecting", m_Name);
		return ReflectInto(outNewLayout);
	}

	void CommitNewLayout(Ref<RenderingPipeline::DescriptorSetLayout> newLayout) {
		m_DescriptorSetLayout = std::move(newLayout);
	}

	bool Reflect() {
		Ref<RenderingPipeline::DescriptorSetLayout> newLayout;
		if (!ReflectInto(newLayout)) {
			return false;
		}
		m_DescriptorSetLayout = std::move(newLayout);
		return true;
	}

	void CreatePipelineLayout(VkDescriptorSetLayout globalLayout) {
		if (m_Layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(m_Device.GetDevice(), m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}
		if (!m_DescriptorSetLayout) {
			AQUILA_LOG_ERROR("Shader '{}': cannot create pipeline layout without descriptor set layout", m_Name);
			return;
		}

		VkDescriptorSetLayout setLayouts[] = { globalLayout, m_DescriptorSetLayout->GetDescriptorSetLayout() };

		VkPushConstantRange pushConstant{};
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstant.offset = 0;
		pushConstant.size = sizeof(glm::mat4) * 2;

		VkPipelineLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.setLayoutCount = 2;
		info.pSetLayouts = setLayouts;
		info.pushConstantRangeCount = 1;
		info.pPushConstantRanges = &pushConstant;

		if (vkCreatePipelineLayout(m_Device.GetDevice(), &info, nullptr, &m_Layout) != VK_SUCCESS) {
			AQUILA_LOG_ERROR("Shader '{}': failed to create pipeline layout", m_Name);
			return;
		}
		AQUILA_LOG_DEBUG("Shader '{}': pipeline layout created", m_Name);
	}

	void Cleanup() {
		for (auto &stage : m_Stages) {
			if (stage.module != VK_NULL_HANDLE) {
				vkDestroyShaderModule(m_Device.GetDevice(), stage.module, nullptr);
				stage.module = VK_NULL_HANDLE;
			}
		}
		m_Stages.clear();
		m_SlangPath.clear();
		m_DescriptorSetLayout.reset();

		if (m_Layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(m_Device.GetDevice(), m_Layout, nullptr);
			m_Layout = VK_NULL_HANDLE;
		}
	}

	void SetTargetFormats(const Helpers::PipelineRenderingFormats &formats) {
		m_TargetFormats = formats;
		AQUILA_LOG_DEBUG("Shader '{}': set target formats ({} color, depth={})", m_Name, formats.colorFormats.size(),
						 formats.depthFormat != VK_FORMAT_UNDEFINED);
	}

	[[nodiscard]] const Helpers::PipelineRenderingFormats &GetTargetFormats() const { return m_TargetFormats; }
	[[nodiscard]] bool IsValid() const { return !m_Stages.empty(); }
	[[nodiscard]] const std::string &GetSlangPath() const { return m_SlangPath; }
	[[nodiscard]] ShaderType GetShaderType() const { return m_ShaderType; }
	void SetShaderType(ShaderType type) { m_ShaderType = type; }
	[[nodiscard]] const std::vector<ReflectedBinding> &GetReflectedBindings() const { return m_ReflectedBindings; }

	[[nodiscard]] VkDescriptorSetLayout GetDescriptorSetLayout() const {
		return m_DescriptorSetLayout ? m_DescriptorSetLayout->GetDescriptorSetLayout() : VK_NULL_HANDLE;
	}

  private:
	Device &m_Device;
	std::string m_SlangPath;
	ShaderType m_ShaderType = ShaderType::Standard;
	Helpers::PipelineRenderingFormats m_TargetFormats;
	std::vector<ReflectedBinding> m_ReflectedBindings;

	struct BindingInfo {
		VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		VkShaderStageFlags stageFlags = 0;
		uint32 descriptorCount = 1;
		bool occupied = false;
	};

	static bool SlangTypeToDescriptor(slang::TypeLayoutReflection *typeLayout, VkDescriptorType &outVkType,
									  ReflectedBindingType &outReflectedType) {
		if ((typeLayout == nullptr) || typeLayout->getBindingRangeCount() == 0) {
			return false;
		}

		switch (typeLayout->getBindingRangeType(0)) {
		case slang::BindingType::CombinedTextureSampler:
		case slang::BindingType::Texture:
		case slang::BindingType::Sampler:
			outVkType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			outReflectedType = ReflectedBindingType::CombinedImageSampler;
			return true;
		case slang::BindingType::ConstantBuffer:
			outVkType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			outReflectedType = ReflectedBindingType::UniformBuffer;
			return true;
		case slang::BindingType::RawBuffer:
		case slang::BindingType::MutableRawBuffer:
		case slang::BindingType::TypedBuffer:
		case slang::BindingType::MutableTypedBuffer:
			outVkType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			outReflectedType = ReflectedBindingType::StorageBuffer;
			return true;
		default:
			return false;
		}
	}

	// Mirrors the inner loop body of SPIRV-Reflect: processes one binding variable
	// and inserts/merges it into the sets map.
	void ProcessBinding(slang::VariableLayoutReflection *var, VkShaderStageFlags stageFlags,
						std::map<uint32, std::map<uint32, BindingInfo>> &sets) {
		if (var == nullptr) {
			return;
		}

		slang::TypeLayoutReflection *typeLayout = var->getTypeLayout();
		if (typeLayout == nullptr) {
			return;
		}

		if (typeLayout->getBindingRangeCount() == 0) {
			uint32 fieldCount = typeLayout->getFieldCount();
			for (uint32 f = 0; f < fieldCount; ++f) {
				ProcessBinding(typeLayout->getFieldByIndex(f), stageFlags, sets);
			}
			return;
		}

		VkDescriptorType vkType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		ReflectedBindingType reflectedType = ReflectedBindingType::Unknown;
		if (!SlangTypeToDescriptor(typeLayout, vkType, reflectedType)) {
			return;
		}

		auto setIndex = (uint32)var->getBindingSpace();
		auto bindingIndex = (uint32)var->getBindingIndex();
		uint32 count = 1;

		slang::TypeReflection *type = var->getType();
		if ((type != nullptr) && type->getKind() == slang::TypeReflection::Kind::Array) {
			count = (uint32)type->getElementCount();
			if (count == 0) {
				count = 1; // unbounded arrays default to 1
			}
		}

		auto &existing = sets[setIndex][bindingIndex];
		if (!existing.occupied) {
			existing = { .descriptorType = vkType,
						 .stageFlags = stageFlags,
						 .descriptorCount = count,
						 .occupied = true };

			ReflectedBinding rb{};
			rb.name = var->getName() ? var->getName() : "";
			rb.set = setIndex;
			rb.bindingIndex = bindingIndex;
			rb.descriptorCount = count;
			rb.stageFlags = stageFlags;
			rb.type = reflectedType;
			m_ReflectedBindings.push_back(rb);
		} else {
			// Same binding seen from another stage — merge stage flags
			existing.stageFlags |= stageFlags;
			for (auto &recorded : m_ReflectedBindings) {
				if (recorded.set == setIndex && recorded.bindingIndex == bindingIndex) {
					recorded.stageFlags |= stageFlags;
				}
			}
		}
	}

	bool ReflectInto(Ref<RenderingPipeline::DescriptorSetLayout> &outLayout) {
		if (m_Stages.empty()) {
			AQUILA_LOG_ERROR("Shader '{}': no stages to reflect", m_Name);
			return false;
		}

		m_ReflectedBindings.clear();
		std::map<uint32, std::map<uint32, BindingInfo>> sets;

		for (const auto &stage : m_Stages) {
			if (!stage.linkedComponent)
				continue;

			slang::ProgramLayout *layout = stage.linkedComponent->getLayout();
			if (!layout)
				continue;

			auto stageFlags = static_cast<VkShaderStageFlags>(stage.stage);

			uint32 paramCount = layout->getParameterCount();
			for (uint32 i = 0; i < paramCount; ++i) {
				ProcessBinding(layout->getParameterByIndex(i), stageFlags, sets);
			}

			slang::TypeLayoutReflection *globalParams = layout->getGlobalParamsTypeLayout();
			if (globalParams != nullptr) {
				uint32 fieldCount = globalParams->getFieldCount();
				for (uint32 f = 0; f < fieldCount; ++f) {
					slang::VariableLayoutReflection *field = globalParams->getFieldByIndex(f);
					if (field == nullptr) {
						continue;
					}
					auto si = (uint32)field->getBindingSpace();
					auto bi = (uint32)field->getBindingIndex();
					if (sets.contains(si) && sets[si].contains(bi)) {
						continue;
					}
					ProcessBinding(field, stageFlags, sets);
				}
			}
		}

		for (auto &rb : m_ReflectedBindings) {
			if (rb.type != ReflectedBindingType::UniformBuffer || rb.set == 0) {
				continue;
			}

			for (const auto &stage : m_Stages) {
				if (stage.linkedComponent == nullptr) {
					continue;
				}
				slang::ProgramLayout *layout = stage.linkedComponent->getLayout();
				if (layout == nullptr) {
					continue;
				}

				bool found = false;
				uint32 paramCount = layout->getParameterCount();
				for (uint32 i = 0; i < paramCount; ++i) {
					slang::VariableLayoutReflection *var = layout->getParameterByIndex(i);
					if (var == nullptr) {
						continue;
					}
					if ((uint32)var->getBindingSpace() != rb.set) {
						continue;
					}
					if ((uint32)var->getBindingIndex() != rb.bindingIndex) {
						continue;
					}

					slang::TypeLayoutReflection *typeLayout = var->getTypeLayout();
					if (typeLayout == nullptr) {
						break;
					}

					slang::TypeLayoutReflection *structLayout = typeLayout->getElementTypeLayout();
					if (structLayout == nullptr) {
						structLayout = typeLayout;
					}

					uint32 fieldCount = structLayout->getFieldCount();
					for (uint32 f = 0; f < fieldCount; ++f) {
						slang::VariableLayoutReflection *field = structLayout->getFieldByIndex(f);
						if ((field == nullptr) || (field->getName() == nullptr)) {
							continue;
						}

						slang::TypeReflection *fieldType = field->getType();
						if (fieldType == nullptr) {
							continue;
						}

						ReflectedBinding::UBOField uboField{};
						uboField.name = field->getName();

						slang::TypeReflection::Kind kind = fieldType->getKind();
						if (kind == slang::TypeReflection::Kind::Vector) {
							switch (fieldType->getColumnCount()) {
							case 4:
								uboField.paramType = Material::ParameterType::Color;
								break;
							case 3:
								uboField.paramType = Material::ParameterType::Vec3;
								break;
							case 2:
								uboField.paramType = Material::ParameterType::Vec2;
								break;
							default:
								continue;
							}
						} else if (kind == slang::TypeReflection::Kind::Scalar) {
							switch (fieldType->getScalarType()) {
							case slang::TypeReflection::ScalarType::Float32:
								uboField.paramType = Material::ParameterType::Float;
								break;
							case slang::TypeReflection::ScalarType::Int32:
							case slang::TypeReflection::ScalarType::UInt32:
								uboField.paramType = Material::ParameterType::Int;
								break;
							default:
								continue;
							}
						} else {
							continue;
						}

						rb.uboFields.push_back(uboField);
						AQUILA_LOG_DEBUG("Shader '{}': reflected UBO field '{}' in cbuffer '{}'", m_Name, uboField.name,
										 rb.name);
					}

					found = true;
					break;
				}
				if (found) {
					break;
				}
			}
		}

		auto builder = RenderingPipeline::DescriptorSetLayout::Builder(m_Device);
		if (sets.contains(1)) {
			for (const auto &[bindingIdx, info] : sets[1]) {
				builder.AddBinding(bindingIdx, info.descriptorType, info.stageFlags, info.descriptorCount);
			}
			AQUILA_LOG_INFO("Shader '{}': reflected {} binding(s) in set 1", m_Name, sets[1].size());
		} else {
			AQUILA_LOG_WARNING("Shader '{}': no descriptor set 1 found, creating empty layout", m_Name);
		}

		outLayout = builder.Build();
		return outLayout != nullptr;
	}

	VkShaderModule CreateShaderModule(const std::vector<uint32> &spirv) {
		VkShaderModuleCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		info.codeSize = spirv.size() * sizeof(uint32);
		info.pCode = spirv.data();

		VkShaderModule mod = nullptr;
		AQUILA_VULKAN_CHECK(vkCreateShaderModule(m_Device.GetDevice(), &info, nullptr, &mod));
		return mod;
	}
};

} // namespace Aquila::Graphics::Shader
#endif
