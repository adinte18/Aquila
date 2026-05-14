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

	// Primary render targets, update these when you write to them.
	Graphics::RG::RGTextureHandle hSceneColor; // RGBA16F scene colour
	Graphics::RG::RGTextureHandle hDepth;	   // Depth32

	uint32 width = 0;
	uint32 height = 0;
	f32 deltaTime = 0.f;

	// Global per-frame GPU data (cameras, time, resolution).
	// Bind GetDescriptorSet(frameSlot) at set 0 in any pipeline that imports FrameData.slang.
	SceneFrameData *frameData = nullptr;
	uint32 frameSlot = 0;
};

} // namespace Aquila::Rendering
