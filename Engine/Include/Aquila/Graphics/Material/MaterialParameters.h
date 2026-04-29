#pragma once
#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::Graphics {

// Typed enum for all parameter kinds a material can hold.
enum class ParameterType : uint8 { Float, Int, Bool, Vec2, Vec3, Vec4, Texture2D, Color };

// Variant covering every concrete value a parameter can take.
// Texture2D entries are bound directly to the descriptor set;
// all other types are packed into the material UBO.
using ParameterValue = std::variant<f32, int, bool, vec2, vec3, vec4, Ref<GFX::GfxTexture>>;

// One reflected or manually-registered shader parameter.
struct MaterialParameter {
	std::string name;
	ParameterType type = ParameterType::Float;
	ParameterValue value;
	ParameterValue defaultValue;

	// Byte offset inside the material UBO (std140 layout).
	// UINT32_MAX means this parameter is a texture, not in the UBO.
	uint32 uboOffset = UINT32_MAX;

	// Descriptor-set binding index for Texture2D parameters.
	// UINT32_MAX for non-texture parameters.
	uint32 textureBinding = UINT32_MAX;
};

} // namespace Aquila::Graphics
