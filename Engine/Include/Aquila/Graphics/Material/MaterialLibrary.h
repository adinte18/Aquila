#ifndef AQUILA_MATERIAL_LIB_H
#define AQUILA_MATERIAL_LIB_H

#include "Aquila/Graphics/Material/Material.h"
#include "Aquila/Graphics/Material/MaterialParameters.h"
#include "Aquila/Graphics/Resources/Texture2D.h"
#include "Aquila/Graphics/Shader/ShaderWatcher.h"

namespace Aquila::Graphics::Material {

class MaterialLibrary {
  private:
	Device &m_Device;
	Shader::ShaderWatcher m_ShaderWatcher;

	std::unordered_map<std::string, Ref<MaterialTemplate>> m_Templates;
	std::unordered_map<std::string, Ref<Material>> m_Materials;
	std::unordered_map<std::string, Ref<Shader::ShaderProgram>> m_ShaderPrograms;

	Ref<Material> m_DefaultMaterial;
	Ref<Material> m_FallbackMaterial;

	Ref<Resources::Texture2D> m_WhiteTexture;
	Ref<Resources::Texture2D> m_NormalTexture;
	Ref<Resources::Texture2D> m_BlackTexture;

	void CreateFallbackTextures();
	void CreateShaderPrograms();
	void CreateDefaultTemplates();
	void CreateDefaultMaterials();

	Delegate<void(const Ref<Material> &)> m_OnMaterialCreated;
	Delegate<void(const Ref<Material> &)> m_OnShaderReloaded;

  public:
	MaterialLibrary(Device &device) : m_Device(device) {}
	~MaterialLibrary() { Shutdown(); }

	void Initialize();
	void Shutdown();

	Ref<Resources::Texture2D> GetFallbackTextureForType(const std::string &propName);

	void RegisterTemplate(const Ref<MaterialTemplate> &tmpl);
	Ref<MaterialTemplate> GetTemplate(const std::string &name);

	Ref<Material> CreateMaterial(const std::string &name, const std::string &templateName);
	Ref<Material> CreateMaterialFromShader(const std::string &name, const Ref<Shader::ShaderProgram> &shader);
	Ref<Material> GetMaterial(const std::string &name);

	Ref<Material> GetDefaultMaterial() const { return m_DefaultMaterial; }
	Ref<Material> GetFallbackMaterial() const { return m_FallbackMaterial; }

	void RegisterMaterial(const Ref<Material> &material);
	bool HasMaterial(const std::string &name) const;

	const auto &GetAllTemplates() const { return m_Templates; }
	const auto &GetAllMaterials() const { return m_Materials; }

	void EnableShaderHotReload(bool enable);

	// Watch a shader program's .slang file for changes.
	void WatchShader(const Ref<Shader::ShaderProgram> &shader);

	// Poll for file changes, recompile affected programs, and re-sync materials.
	void TickHotReload();

	void SetMaterialCreatedCallback(Delegate<void(const Ref<Material> &)> callback) { m_OnMaterialCreated = callback; }
	void SetShaderReloadedCallback(Delegate<void(const Ref<Material> &)> callback) { m_OnShaderReloaded = callback; }
};

} // namespace Aquila::Graphics::Material
#endif
