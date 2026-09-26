#include <doctest.h>

#include "Aquila/Foundation/Math/Math.h"
#include "Aquila/Rendering/CameraController.h"

#include "CanvasTestSupport.h"

using namespace Aquila;
using namespace Aquila::Rendering;

TEST_SUITE("CameraController") {
	TEST_CASE("a transform with the editor camera's pose gives the same view") {
		CameraController controller;
		const RenderView editor = controller.get_render_view();
		const Mat4 world = glm::translate(Mat4(1.F), Vec3(0.F, 1.5F, -5.F));
		const RenderView from_transform = controller.render_view_from(world, controller.get_fov(), 0.1F, 1000.F);

		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				CHECK(from_transform.view[column][row] == doctest::Approx(editor.view[column][row]).epsilon(1e-4));
				CHECK(from_transform.projection[column][row] ==
					  doctest::Approx(editor.projection[column][row]).epsilon(1e-4));
			}
		}
	}

	TEST_CASE("scale on the transform does not leak into the view") {
		CameraController controller;
		const Mat4 world = glm::scale(glm::translate(Mat4(1.F), Vec3(2.F, 0.F, 0.F)), Vec3(3.F));
		const RenderView view = controller.render_view_from(world, 60.F, 0.1F, 100.F);
		CHECK(Math::length(Vec3(view.view[0])) == doctest::Approx(1.F));
		CHECK(view.position.x == doctest::Approx(2.F));
	}

	TEST_CASE("fov is clamped and applied to the projection") {
		CanvasSingletonsScope singletons;
		CameraController controller;
		const F32 before = controller.get_render_view().projection[1][1];
		controller.set_fov(30.F);
		CHECK(controller.get_fov() == doctest::Approx(30.F));
		CHECK(std::abs(controller.get_render_view().projection[1][1]) > std::abs(before));
		controller.set_fov(500.F);
		CHECK(controller.get_fov() == doctest::Approx(179.F));
	}
}
