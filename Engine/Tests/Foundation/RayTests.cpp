#include <doctest.h>

#include "Aquila/Foundation/Math/Geometry/Ray.h"

using Aquila::Math::Geometry::Ray::Ray;

TEST_SUITE("Ray") {
	TEST_CASE("the closest point on a line is found from any ray") {
		const Ray ray(Vec3(3.F, 5.F, 0.F), Vec3(0.F, -1.F, 0.F));
		const Option<F32> param = ray.closest_param_on_line(Vec3(0.F), Vec3(1.F, 0.F, 0.F));
		REQUIRE(param.has_value());
		CHECK(*param == doctest::Approx(3.F));
	}

	TEST_CASE("a ray parallel to the line has no closest parameter") {
		const Ray ray(Vec3(0.F, 1.F, 0.F), Vec3(1.F, 0.F, 0.F));
		CHECK_FALSE(ray.closest_param_on_line(Vec3(0.F), Vec3(1.F, 0.F, 0.F)).has_value());
	}

	TEST_CASE("plane hits are in front of the ray only") {
		const Ray down(Vec3(2.F, 4.F, 1.F), Vec3(0.F, -1.F, 0.F));
		const Option<Vec3> hit = down.hit_plane(Vec3(0.F), Vec3(0.F, 1.F, 0.F));
		REQUIRE(hit.has_value());
		CHECK(hit->x == doctest::Approx(2.F));
		CHECK(hit->y == doctest::Approx(0.F));

		const Ray up(Vec3(0.F, 4.F, 0.F), Vec3(0.F, 1.F, 0.F));
		CHECK_FALSE(up.hit_plane(Vec3(0.F), Vec3(0.F, 1.F, 0.F)).has_value());
	}
}
