#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/RenderGraph/RGTypes.h"

namespace Aquila::SceneManagement {
class Scene;
}

namespace Aquila::Rendering {

class SceneFrameData;

struct FrameContext {
	SceneManagement::Scene *scene = nullptr;

	mat4 view = mat4(1.f);
	mat4 projection = mat4(1.f);
	mat4 viewProjection = mat4(1.f);
	vec3 cameraPosition = {};

	Graphics::RG::RGTextureHandle hSceneColor;
	Graphics::RG::RGTextureHandle hDepth;

	Graphics::RG::RGBufferHandle hClusterAABBs;
	Graphics::RG::RGBufferHandle hLightList;
	Graphics::RG::RGBufferHandle hClusterLightInfo;

	uint32 width = 0;
	uint32 height = 0;
	f32 deltaTime = 0.f;

	SceneFrameData *frameData = nullptr;
	uint32 frameSlot = 0;
};

} // namespace Aquila::Rendering
