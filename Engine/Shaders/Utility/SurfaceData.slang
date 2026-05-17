#ifndef AQUILA_SURFACE_DATA_SLANG
#define AQUILA_SURFACE_DATA_SLANG

namespace Aquila::Shading {

struct SurfaceData {
	float4 albedo;	 // rgb=tint, a=opacity
	float4 emissive; // rgb=color, a=intensity
	float metallic;
	float roughness;
	float normalStrength;
	float aoStrength;
	float4 extra0;
	float4 extra1;
};

[vk::binding(3, 0)] StructuredBuffer<SurfaceData> materials;

SurfaceData GetSurface(uint materialIndex) {
	return materials[materialIndex];
}

} // namespace Aquila::Shading
#endif // AQUILA_SURFACE_DATA_SLANG
