#include "Aquila/Graphics/Shader/ReloadablePipeline.h"
#include "Aquila/Graphics/Shader/ShaderHotReload.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/Foundation/Macros.h"

namespace Aquila::Graphics::Shader {

ReloadablePipeline::ReloadablePipeline(GFX::GfxContext &ctx, std::string shader_path, BuildFn build)
	: m_ctx(ctx), m_shader_path(std::move(shader_path)), m_build(std::move(build)) {
	m_pipeline = m_build(m_ctx);

#ifdef AQUILA_HOT_RELOAD_ALL_SHADERS
	m_watch_id = ShaderHotReload::get()->register_reloadable(m_shader_path, [this]() { rebuild(); });
#endif
}

ReloadablePipeline::~ReloadablePipeline() {
#ifdef AQUILA_HOT_RELOAD_ALL_SHADERS
	if (m_watch_id != 0 && ShaderHotReload::is_alive()) {
		ShaderHotReload::get()->unregister(m_watch_id);
	}
#endif
}

Ref<ReloadablePipeline> ReloadablePipeline::create(GFX::GfxContext &ctx, std::string shader_path, BuildFn build) {
	return Ref<ReloadablePipeline>(new ReloadablePipeline(ctx, std::move(shader_path), std::move(build)));
}

void ReloadablePipeline::rebuild() {
	Ref<GFX::GfxPipeline> new_pipeline = m_build(m_ctx);
	if (!new_pipeline) {
		AQUILA_LOG_ERROR("ReloadablePipeline: rebuild failed for '{}', keeping previous pipeline", m_shader_path);
		return;
	}

	m_ctx.wait_idle();
	m_pipeline = std::move(new_pipeline);
	AQUILA_LOG_INFO("ReloadablePipeline: reloaded '{}'", m_shader_path);
}

} // namespace Aquila::Graphics::Shader
