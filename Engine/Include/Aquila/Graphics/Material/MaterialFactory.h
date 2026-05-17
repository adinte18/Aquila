#pragma once
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Shader/ShaderWatcher.h"
#include "Aquila/RHI/Backend/RHITypes.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Aquila::GFX {
class GfxContext;
}

namespace Aquila::Graphics::Shader {
class ShaderProgram;
}

namespace Aquila::Graphics {

struct MaterialCreateInfo {
	MaterialType type = MaterialType::PBR;

	std::vector<RHI::TextureFormat> colorFormats = { RHI::TextureFormat::RGBA16F };
	RHI::TextureFormat depthFormat = RHI::TextureFormat::Depth32;

	RHI::CullMode cullMode = RHI::CullMode::Back;
	RHI::FrontFace frontFace = RHI::FrontFace::Clockwise;

	bool depthTest = true;
	bool depthWrite = true;

	bool blendEnabled = false;

	uint32 pushConstantSize = 256;
};

class MaterialFactory : public Foundation::Singleton<MaterialFactory> {
  public:
	Ref<Material> Create(GFX::GfxContext &ctx, const std::string &shaderPath, MaterialCreateInfo info);

	void Tick(GFX::GfxContext &ctx);

	void EnableHotReload(bool enable) { m_Watcher.Enable(enable); }
	[[nodiscard]] bool IsHotReloadEnabled() const { return m_Watcher.IsEnabled(); }

  private:
	struct Entry {
		Ref<Shader::ShaderProgram> program;
		MaterialCreateInfo info;
		std::vector<WeakRef<Material>> instances;
	};

	std::unordered_map<std::string, Entry> m_Entries;
	Shader::ShaderWatcher m_Watcher;

	static Ref<GFX::GfxPipeline> BuildPipeline(GFX::GfxContext &ctx, Shader::ShaderProgram &program,
											   const MaterialCreateInfo &info);

	void RebuildEntry(GFX::GfxContext &ctx, Entry &entry, const std::string &shaderPath);
};

} // namespace Aquila::Graphics
