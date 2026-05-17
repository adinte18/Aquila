#include "Aquila/Graphics/Shader/ShaderProgram.h"
#include "Aquila/RHI/Vulkan/VulkanShaderCompiler.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Foundation/Macros.h"
#include <slang/slang.h>

namespace Aquila::Graphics::Shader {

static RHI::ShaderStageFlags VkStageToRHI(VkShaderStageFlagBits stage) {
	switch (stage) {
	case VK_SHADER_STAGE_VERTEX_BIT:
		return RHI::ShaderStageFlags::Vertex;
	case VK_SHADER_STAGE_FRAGMENT_BIT:
		return RHI::ShaderStageFlags::Fragment;
	case VK_SHADER_STAGE_COMPUTE_BIT:
		return RHI::ShaderStageFlags::Compute;
	case VK_SHADER_STAGE_GEOMETRY_BIT:
		return RHI::ShaderStageFlags::Geometry;
	default:
		return RHI::ShaderStageFlags::Vertex;
	}
}

static Graphics::ParameterType SlangScalarToParamType(slang::TypeReflection *type) {
	using Kind = slang::TypeReflection::Kind;
	using Scalar = slang::TypeReflection::ScalarType;

	Kind kind = type->getKind();
	if (kind != Kind::Scalar && kind != Kind::Vector) {
		return Graphics::ParameterType::Float;
	}

	int cols = type->getColumnCount();
	switch (type->getScalarType()) {
	case Scalar::Float32:
		if (cols == 1) {
			return Graphics::ParameterType::Float;
		}
		if (cols == 2) {
			return Graphics::ParameterType::Vec2;
		}
		if (cols == 3) {
			return Graphics::ParameterType::Vec3;
		}
		if (cols == 4) {
			return Graphics::ParameterType::Vec4;
		}
		break;
	case Scalar::Int32:
		return Graphics::ParameterType::Int;
	case Scalar::Bool:
		return Graphics::ParameterType::Bool;
	default:
		break;
	}
	return Graphics::ParameterType::Float;
}

bool ShaderProgram::AddStageFromSlang(const std::string &slangPath, std::string &errorLog) {
	m_SlangPath = slangPath;

	std::vector<RHI::VulkanCompiledStage> compiled;
	if (!RHI::VulkanShaderCompiler::CompileFile(slangPath, compiled, errorLog)) {
		return false;
	}

	for (auto &stage : compiled) {
		ShaderStage s;
		s.stage = VkStageToRHI(stage.stage);
		s.spirv = std::move(stage.spirv);
		s.entryPointName = std::move(stage.entryPointName);
		s.linkedComponent = std::move(stage.linkedComponent);
		s.session = std::move(stage.session);
		m_Stages.push_back(std::move(s));
	}
	return true;
}

bool ShaderProgram::Reload(std::string &errorLog, Ref<GFX::GfxDescriptorSetLayout> &outNewLayout) {
	if (m_SlangPath.empty()) {
		errorLog = "ShaderProgram: no source path set";
		return false;
	}

	std::vector<RHI::VulkanCompiledStage> compiled;
	if (!RHI::VulkanShaderCompiler::CompileFile(m_SlangPath, compiled, errorLog)) {
		return false;
	}

	m_Stages.clear();
	for (auto &stage : compiled) {
		ShaderStage s;
		s.stage = VkStageToRHI(stage.stage);
		s.spirv = std::move(stage.spirv);
		s.entryPointName = std::move(stage.entryPointName);
		s.linkedComponent = std::move(stage.linkedComponent);
		s.session = std::move(stage.session);
		m_Stages.push_back(std::move(s));
	}

	return ReflectInto(outNewLayout);
}

void ShaderProgram::CommitNewLayout(Ref<GFX::GfxDescriptorSetLayout> newLayout) {
	m_DescriptorSetLayout = std::move(newLayout);
}

bool ShaderProgram::Reflect() {
	return ReflectInto(m_DescriptorSetLayout);
}

void ShaderProgram::Cleanup() {
	m_Stages.clear();
	m_ReflectedBindings.clear();
	m_DescriptorSetLayout.reset();
}

RHI::ShaderStageDesc ShaderProgram::GetStageDesc(RHI::ShaderStageFlags stage) const {
	for (const auto &s : m_Stages) {
		if (s.stage == stage) {
			return { .stage = s.stage, .spirv = s.spirv, .entryPoint = s.entryPointName };
		}
	}
	return {};
}

bool ShaderProgram::SlangTypeToDescriptor(slang::TypeLayoutReflection *typeLayout, RHI::DescriptorType &outType,
										  ReflectedBindingType &outReflectedType) {
	if (!typeLayout) {
		return false;
	}
	slang::TypeReflection *type = typeLayout->getType();
	if (!type) {
		return false;
	}

	using Kind = slang::TypeReflection::Kind;
	switch (type->getKind()) {
	case Kind::ConstantBuffer:
	case Kind::ParameterBlock:
		outType = RHI::DescriptorType::UniformBuffer;
		outReflectedType = ReflectedBindingType::UniformBuffer;
		return true;

	case Kind::Resource: {
		SlangResourceShape base = (SlangResourceShape)(type->getResourceShape() & SLANG_RESOURCE_BASE_SHAPE_MASK);

		if (base == SLANG_TEXTURE_1D || base == SLANG_TEXTURE_2D || base == SLANG_TEXTURE_3D ||
			base == SLANG_TEXTURE_CUBE) {
			outType = RHI::DescriptorType::CombinedImageSampler;
			outReflectedType = ReflectedBindingType::CombinedImageSampler;
			return true;
		}
		if (base == SLANG_STRUCTURED_BUFFER || base == SLANG_BYTE_ADDRESS_BUFFER) {
			outType = RHI::DescriptorType::StorageBuffer;
			outReflectedType = ReflectedBindingType::StorageBuffer;
			return true;
		}
		return false;
	}

	case Kind::SamplerState:

		return false;

	default:
		return false;
	}
}

void ShaderProgram::ProcessBinding(slang::VariableLayoutReflection *var, RHI::ShaderStageFlags stageFlags,
								   std::map<uint32, std::map<uint32, BindingInfo>> &sets) {
	if (!var) {
		return;
	}

	uint32 bindingIndex = (uint32)var->getBindingIndex();
	uint32 set = (uint32)var->getBindingSpace();

	slang::TypeLayoutReflection *typeLayout = var->getTypeLayout();
	if (!typeLayout) {
		return;
	}

	RHI::DescriptorType descType{};
	ReflectedBindingType reflType{};
	if (!SlangTypeToDescriptor(typeLayout, descType, reflType)) {
		return;
	}

	auto &bi = sets[set][bindingIndex];
	bi.descriptorType = descType;
	bi.stageFlags = bi.stageFlags | stageFlags;
	bi.descriptorCount = 1;
	bi.occupied = true;

	ReflectedBinding rb{};
	rb.name = var->getName() ? var->getName() : "";
	rb.set = set;
	rb.bindingIndex = bindingIndex;
	rb.descriptorCount = 1;
	rb.type = reflType;
	rb.stageFlags = stageFlags;

	if (reflType == ReflectedBindingType::UniformBuffer) {
		slang::TypeLayoutReflection *inner = typeLayout->getElementTypeLayout();
		if (inner) {
			for (uint32 f = 0; f < (uint32)inner->getFieldCount(); ++f) {
				slang::VariableLayoutReflection *field = inner->getFieldByIndex(f);
				if (!field || !field->getName()) {
					continue;
				}
				slang::TypeReflection *ft = field->getTypeLayout() ? field->getTypeLayout()->getType() : nullptr;
				if (!ft) {
					continue;
				}
				ReflectedBinding::UBOField uf{};
				uf.name = field->getName();
				uf.paramType = SlangScalarToParamType(ft);
				rb.uboFields.push_back(std::move(uf));
			}
		}
	}

	m_ReflectedBindings.push_back(std::move(rb));
}

bool ShaderProgram::ReflectInto(Ref<GFX::GfxDescriptorSetLayout> &outLayout) {
	if (m_Stages.empty()) {
		return false;
	}

	m_ReflectedBindings.clear();
	std::map<uint32, std::map<uint32, BindingInfo>> sets;

	const auto &primary = m_Stages[0];
	if (!primary.linkedComponent) {
		outLayout = nullptr;
		return true;
	}

	slang::ProgramLayout *pl = primary.linkedComponent->getLayout();
	if (!pl) {
		outLayout = nullptr;
		return true;
	}

	for (uint32 i = 0; i < (uint32)pl->getParameterCount(); ++i) {
		ProcessBinding(pl->getParameterByIndex(i), primary.stage, sets);
	}

	for (size_t si = 1; si < m_Stages.size(); ++si) {
		const auto &stage = m_Stages[si];
		if (!stage.linkedComponent) {
			continue;
		}
		slang::ProgramLayout *spl = stage.linkedComponent->getLayout();
		if (!spl) {
			continue;
		}
		for (uint32 i = 0; i < (uint32)spl->getParameterCount(); ++i) {
			auto *var = spl->getParameterByIndex(i);
			if (!var) {
				continue;
			}
			uint32 b = (uint32)var->getBindingIndex();
			uint32 s = (uint32)var->getBindingSpace();
			if (sets.count(s) && sets[s].count(b)) {
				sets[s][b].stageFlags = sets[s][b].stageFlags | stage.stage;
			}
		}
	}

	auto it = sets.find(1);
	if (it == sets.end()) {
		outLayout = nullptr;
		return true;
	}

	RHI::DescriptorSetLayoutDesc desc{};
	for (auto &[idx, info] : it->second) {
		if (!info.occupied) {
			continue;
		}
		desc.bindings.push_back({
			.binding = idx,
			.type = info.descriptorType,
			.stages = info.stageFlags,
			.count = info.descriptorCount,
		});
	}

	outLayout = m_Context.CreateDescriptorSetLayout(desc);
	return true;
}

} // namespace Aquila::Graphics::Shader
