#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Graphics/RenderGraph/RGTypes.h"

namespace Aquila::SceneManagement {
class Scene;
}

namespace Aquila::GFX {
class GfxSwapchain;
}

namespace Aquila::Rendering {

class SceneFrameData;

struct FrameContext {
	SceneManagement::Scene *scene = nullptr;

	Mat4 view = Mat4(1.F);
	Mat4 projection = Mat4(1.F);
	Mat4 view_projection = Mat4(1.F);
	Vec3 camera_position = {};

	Graphics::RG::RGTextureHandle h_scene_color;
	Graphics::RG::RGTextureHandle h_depth;

	Graphics::RG::RGBufferHandle h_cluster_aab_bs;
	Graphics::RG::RGBufferHandle h_light_list;
	Graphics::RG::RGBufferHandle h_cluster_light_info;

	GFX::GfxSwapchain *swapchain = nullptr;
	Uint32 swapchain_image_index = 0;

	Uint32 width = 0;
	Uint32 height = 0;
	F32 delta_time = 0.F;

	SceneFrameData *frame_data = nullptr;
	Uint32 frame_slot = 0;
};

} // namespace Aquila::Rendering
