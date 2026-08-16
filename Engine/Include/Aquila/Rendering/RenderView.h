#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::Rendering {

struct RenderView {
	Mat4 view = Mat4(1.F);
	Mat4 projection = Mat4(1.F);
	Vec3 position = Vec3(0.F);
	Vec3 forward = Vec3(0.F, 0.F, 1.F);
	Vec3 up = Vec3(0.F, 1.F, 0.F);
	Vec3 right = Vec3(1.F, 0.F, 0.F);
	F32 near_plane = 0.1F;
	F32 far_plane = 1000.F;
	F32 fov = 60.F;
	F32 aspect = 1.F;
	bool is_orthographic = false;
	bool valid = false;
};

} // namespace Aquila::Rendering
