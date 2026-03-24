#ifndef ENVIRONMENT_BAKER_H
#define ENVIRONMENT_BAKER_H

#include "Aquila/Scene/Components/SkyLightComponent.h"
#include "Aquila/Graphics/Resources/Texture2D.h"
#include "Aquila/Foundation/Math/Math.h"

namespace Aquila::Rendering {

// https://www.ppsloan.org/publications/shillum_final23.pdf
// https://cseweb.ucsd.edu/~ravir/papers/envmap/envmap.pdf
class EnvironmentBaker {
  public:
	static SceneManagement::Components::SHCoefficients BakeToSH(const Ref<Graphics::Resources::Texture2D> &hdrTexture,
																int numSamples = 10000) {
		SceneManagement::Components::SHCoefficients sphericalHarmonics;

		if (!hdrTexture || (hdrTexture->GetData() == nullptr)) {
			AQUILA_LOG_ERROR("Failed to bake environment: Invalid HDR texture");
			return sphericalHarmonics;
		}

		const void *data = hdrTexture->GetData();
		uint32 width = hdrTexture->GetWidth();
		uint32 height = hdrTexture->GetHeight();

		AQUILA_LOG_INFO("Baking environment '{}' ({}x{}) with {} samples", hdrTexture->GetPath(), width, height,
						numSamples);

		// Initialize coefficients to zero
		for (int i = 0; i < 9; ++i) {
			sphericalHarmonics.coeffs[i] = vec3(0.0f);
		}

		std::default_random_engine rng(12345);
		std::uniform_real_distribution<f32> dist(0.0f, 1.0f);

		// Monte Carlo integration over the sphere
		for (int sample = 0; sample < numSamples; ++sample) {
			f32 u1 = dist(rng);
			f32 u2 = dist(rng);

			// Uniform sampling on sphere
			f32 theta = 2.0f * Math::PI * u1;
			f32 phi = acos(2.0f * u2 - 1.0f);

			vec3 dir(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi));

			// Sample environment map at this direction
			vec3 color = SampleEnvironmentMap(reinterpret_cast<const f32 *>(data), width, height, dir);

			// Evaluate SH basis functions for this direction
			// The paper I cited above proves that "a 9D subspace suffices"
			std::array<f32, 9> shBasis{};
			EvaluateSHBasis(dir, shBasis);

			for (int i = 0; i < 9; ++i) {
				sphericalHarmonics.coeffs[i] += color * shBasis[i];
			}
		}

		// Normalize by number of samples and solid angle (4pi)
		f32 weight = 4.0f * Math::PI / static_cast<f32>(numSamples);
		for (int i = 0; i < 9; ++i) {
			sphericalHarmonics.coeffs[i] *= weight;
		}

		AQUILA_LOG_INFO("Environment baking complete");
		return sphericalHarmonics;
	}

	// Sample environment map at a given direction
	static vec3 SampleEnvironmentMap(const f32 *data, int width, int height, const vec3 &dir) {
		// Convert direction to lat-long UV coordinates
		f32 phi = atan2(dir.z, dir.x);
		f32 theta = acos(Math::Clamp(dir.y, -1.0f, 1.0f));

		f32 u = (phi + Math::PI) / (2.0f * Math::PI);
		f32 v = theta / Math::PI;

		// Clamp and convert to pixel coordinates
		int x = static_cast<int>(u * width) % width;
		int y = static_cast<int>(v * height) % height;

		if (x < 0) {
			x += width;
		}
		if (y < 0) {
			y += height;
		}

		int idx = (y * width + x) * 4; // HDR is loaded in 4 channels
		return { data[idx], data[idx + 1], data[idx + 2] };
	}

	//https://beatthezombie.github.io/sh_post_1/
	//https://github.com/google/spherical-harmonics/blob/master/sh/spherical_harmonics.cc
	//https://mathworld.wolfram.com/Condon-ShortleyPhase.html
	static void EvaluateSHBasis(const vec3 &dir, std::array<f32, 9> &basis) {
		// Constants for normalization
		const f32 c0 = 0.282095f; // 1 / (2 * sqrt(pi))
		const f32 c1 = 0.488603f; // sqrt(3 / (4pi))
		const f32 c2 = 1.092548f; // sqrt(15 / (4pi))
		const f32 c3 = 0.315392f; // sqrt(5 / (16pi))
		const f32 c4 = 0.546274f; // sqrt(15 / (16pi))

		// Band 0 (l=0, m=0) - DC
		basis[0] = c0; // Y_0_0

		// Band 1 (l=1) - Linear
		basis[1] = -c1 * dir.y; // Y_1_-1
		basis[2] = c1 * dir.z;	// Y_1_0
		basis[3] = -c1 * dir.x; // Y_1_1

		// Band 2 (l=2) - Quadratic
		basis[4] = c2 * dir.x * dir.y;					 // Y_2_-2
		basis[5] = -c2 * dir.y * dir.z;					 // Y_2_-1
		basis[6] = c3 * (3.0f * dir.z * dir.z - 1.0f);	 // Y_2_0
		basis[7] = -c2 * dir.x * dir.z;					 // Y_2_1
		basis[8] = c4 * (dir.x * dir.x - dir.y * dir.y); // Y_2_2
	}
};

} // namespace Aquila::Rendering

#endif // ENVIRONMENT_BAKER_H
