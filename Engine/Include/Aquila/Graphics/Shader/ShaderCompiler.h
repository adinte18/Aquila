#ifndef AQUILA_SHADER_COMPILER_H
#define AQUILA_SHADER_COMPILER_H

#include "Aquila/Core/AquilaCore.h"
#include "Aquila/Platform/Filesystem/VirtualFileSystem.h"
#include <slang/slang-com-ptr.h>
#include <slang/slang.h>

namespace Aquila::Graphics::Shader {

struct CompiledStage {
	VkShaderStageFlagBits stage{};
	std::vector<uint32> spirv;
	std::string entryPointName;
	Slang::ComPtr<slang::IComponentType> linkedComponent;
	Slang::ComPtr<slang::ISession> session;
};

class ShaderCompiler {
  public:
	static void Initialize() {
		if (s_Initialized) {
			return;
		}
		slang::createGlobalSession(s_GlobalSession.writeRef());
		s_Initialized = true;
	}

	static void Shutdown() {
		s_GlobalSession = nullptr;
		s_Initialized = false;
	}

	static bool CompileFile(const std::string &filepath, std::vector<CompiledStage> &outStages, std::string &errorLog) {
		AQUILA_ASSERT(s_Initialized, "ShaderCompiler::Initialize() was not called");

		slang::TargetDesc targetDesc{};
		targetDesc.format = SLANG_SPIRV;
		targetDesc.profile = s_GlobalSession->findProfile("spirv_1_0");

		slang::SessionDesc sessionDesc{};
		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;

		Slang::ComPtr<slang::ISession> session;
		if (SLANG_FAILED(s_GlobalSession->createSession(sessionDesc, session.writeRef()))) {
			errorLog = "Failed to create Slang session";
			return false;
		}

		std::string sourceCode;
		const bool isVirtualPath = filepath.find("://") != std::string::npos;
		if (isVirtualPath) {
			auto *vfs = Platform::Filesystem::VirtualFileSystem::Get();
			sourceCode = vfs->ReadTextFile(filepath);
			if (sourceCode.empty()) {
				errorLog = "VFS: failed to read shader source from '" + filepath + "'";
				return false;
			}
		} else {
			std::ifstream file(filepath);
			if (!file.is_open()) {
				errorLog = "Failed to open shader file '" + filepath + "'";
				return false;
			}
			sourceCode = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
		}

		const std::string moduleName = filepath;

		slang::IModule *slangModule = nullptr;
		{
			Slang::ComPtr<slang::IBlob> diagnostics;
			slangModule = session->loadModuleFromSourceString(moduleName.c_str(), // logical module name
															  filepath.c_str(),	  // path shown in diagnostics
															  sourceCode.c_str(), diagnostics.writeRef());
			if (slangModule == nullptr) {
				errorLog = (diagnostics != nullptr) ? static_cast<const char *>(diagnostics->getBufferPointer())
													: "Failed to load Slang module from '" + filepath + "'";
				return false;
			}
		}

		int32 entryPointCount = slangModule->getDefinedEntryPointCount();
		if (entryPointCount == 0) {
			errorLog = "No entry points found in '" + filepath +
					   "' — ensure functions are marked with [shader(\"vertex\")] etc.";
			return false;
		}

		bool anyFailed = false;
		for (int32 ep = 0; ep < entryPointCount; ++ep) {
			Slang::ComPtr<slang::IEntryPoint> entryPoint;
			if (SLANG_FAILED(slangModule->getDefinedEntryPoint(ep, entryPoint.writeRef()))) {
				AQUILA_LOG_WARNING("ShaderCompiler: failed to get entry point {} from '{}'", ep, filepath);
				anyFailed = true;
				continue;
			}

			slang::IComponentType *components[] = { slangModule, entryPoint };
			Slang::ComPtr<slang::IComponentType> composed;
			session->createCompositeComponentType(components, 2, composed.writeRef());

			slang::ProgramLayout *layout = composed->getLayout();
			slang::EntryPointReflection *epReflect = layout->getEntryPointByIndex(0);
			VkShaderStageFlagBits vkStage = SlangStageToVkStage(epReflect->getStage());
			std::string epName = epReflect->getName();

			Slang::ComPtr<slang::IComponentType> linked;
			{
				Slang::ComPtr<slang::IBlob> diagnostics;
				composed->link(linked.writeRef(), diagnostics.writeRef());
				if (linked == nullptr) {
					errorLog = (diagnostics != nullptr) ? static_cast<const char *>(diagnostics->getBufferPointer())
														: "Link failed for entry point '" + epName + "'";
					anyFailed = true;
					continue;
				}
			}

			Slang::ComPtr<slang::IBlob> spirvBlob;
			{
				Slang::ComPtr<slang::IBlob> diagnostics;
				linked->getEntryPointCode(0, 0, spirvBlob.writeRef(), diagnostics.writeRef());
				if (spirvBlob == nullptr) {
					errorLog = (diagnostics != nullptr) ? static_cast<const char *>(diagnostics->getBufferPointer())
														: "Code generation failed for entry point '" + epName + "'";
					anyFailed = true;
					continue;
				}
			}

			CompiledStage result;
			result.stage = vkStage;
			result.entryPointName = epName;
			result.linkedComponent = linked;
			result.session = session;
			const auto *data = static_cast<const uint32 *>(spirvBlob->getBufferPointer());
			result.spirv.assign(data, data + (spirvBlob->getBufferSize() / sizeof(uint32)));
			outStages.push_back(std::move(result));
		}

		if (outStages.empty()) {
			if (errorLog.empty()) {
				errorLog = "All entry points failed to compile in '" + filepath + "'";
			}
			return false;
		}

		if (anyFailed) {
			outStages.clear();
			return false;
		}

		return true;
	}

	static VkShaderStageFlagBits SlangStageToVkStage(SlangStage stage) {
		switch (stage) {
		case SLANG_STAGE_VERTEX:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case SLANG_STAGE_FRAGMENT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case SLANG_STAGE_COMPUTE:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		case SLANG_STAGE_GEOMETRY:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		default:
			AQUILA_LOG_WARNING("ShaderCompiler: unhandled SlangStage {}, defaulting to VERTEX",
							   static_cast<int>(stage));
			return VK_SHADER_STAGE_VERTEX_BIT;
		}
	}

  private:
	static inline Slang::ComPtr<slang::IGlobalSession> s_GlobalSession;
	static inline bool s_Initialized = false;
};

} // namespace Aquila::Graphics::Shader
#endif
