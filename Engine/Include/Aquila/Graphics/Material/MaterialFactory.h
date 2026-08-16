#pragma once
#include "Aquila/Foundation/Singleton.h"
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/Material/Material.h"
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

	std::vector<RHI::TextureFormat> color_formats = { RHI::TextureFormat::RGBA16F };
	RHI::TextureFormat depth_format = RHI::TextureFormat::Depth32;

	RHI::CullMode cull_mode = RHI::CullMode::Back;
	RHI::FrontFace front_face = RHI::FrontFace::Clockwise;

	bool depth_test = true;
	bool depth_write = true;

	bool blend_enabled = false;

	Uint32 push_constant_size = 256;
};

class MaterialFactory : public Foundation::Singleton<MaterialFactory> {
  public:
	~MaterialFactory();

	Ref<Material> create(GFX::GfxContext &ctx, const std::string &shader_path, MaterialCreateInfo info);

  private:
	struct Entry {
		Ref<Shader::ShaderProgram> program;
		MaterialCreateInfo info;
		std::vector<WeakRef<Material>> instances;
		Uint64 watch_id = 0;
	};

	std::unordered_map<std::string, Entry> m_entries;

	static Ref<GFX::GfxPipeline> build_pipeline(GFX::GfxContext &ctx, Shader::ShaderProgram &program,
											   const MaterialCreateInfo &info);

	void rebuild_entry(GFX::GfxContext &ctx, Entry &entry, const std::string &shader_path);
};

} // namespace Aquila::Graphics
