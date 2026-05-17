#ifndef AQUILA_ENVIRONMENT_DATA_SLANG
#define AQUILA_ENVIRONMENT_DATA_SLANG

namespace Aquila::Shading {

struct EnvironmentData {
	float4 shCoeffs[9];		 // spherical harmonic coefficients (RGB in xyz, w unused)
	float4 tintAndIntensity; // xyz = tint, w = intensity
	int enabled;
	int3 _pad;
};

[vk::binding(2, 0)] ConstantBuffer<EnvironmentData> environment;

bool IsEnvironmentEnabled() {
	return environment.enabled != 0;
}
float3 GetEnvironmentTint() {
	return environment.tintAndIntensity.xyz;
}
float GetEnvironmentIntensity() {
	return environment.tintAndIntensity.w;
}
float4 GetSHCoeff(uint index) {
	return environment.shCoeffs[index];
}

// Evaluate irradiance from 9-coefficient (L0+L1+L2) SH for surface normal N.
float3 EvaluateSH(float3 N) {
	const float c0 = 0.282095;
	const float c1 = 0.488603;
	const float c2 = 1.092548;
	const float c3 = 0.315392;
	const float c4 = 0.546274;

	float3 result = c0 * environment.shCoeffs[0].xyz - c1 * N.y * environment.shCoeffs[1].xyz +
					c1 * N.z * environment.shCoeffs[2].xyz - c1 * N.x * environment.shCoeffs[3].xyz +
					c2 * N.x * N.y * environment.shCoeffs[4].xyz - c2 * N.y * N.z * environment.shCoeffs[5].xyz +
					c3 * (3.0 * N.z * N.z - 1.0) * environment.shCoeffs[6].xyz -
					c2 * N.x * N.z * environment.shCoeffs[7].xyz +
					c4 * (N.x * N.x - N.y * N.y) * environment.shCoeffs[8].xyz;

	return max(result, float3(0.0)) * environment.tintAndIntensity.w * environment.tintAndIntensity.xyz;
}

} // namespace Aquila::Shading
#endif // AQUILA_ENVIRONMENT_DATA_SLANG
