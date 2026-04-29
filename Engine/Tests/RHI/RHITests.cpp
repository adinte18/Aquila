#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <GLFW/glfw3.h>

#include "Aquila/GFX/GfxContext.h"

using namespace Aquila;

namespace {

struct GfxFixture {
	GLFWwindow *window = nullptr;
	Unique<GFX::GfxContext> ctx;

	GfxFixture() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		window = glfwCreateWindow(800, 600, "RHITests", nullptr, nullptr);
		REQUIRE(window != nullptr);
		ctx = GFX::GfxContext::Create(*window);
		REQUIRE(ctx != nullptr);
	}

	~GfxFixture() {
		ctx->WaitIdle();
		ctx.reset();
		glfwDestroyWindow(window);
		glfwTerminate();
	}
};

static GfxFixture *g_Fixture = nullptr;

GfxFixture &Fixture() {
	if (!g_Fixture) {
		g_Fixture = new GfxFixture();
	}
	return *g_Fixture;
}

GFX::GfxContext &Ctx() {
	return *Fixture().ctx;
}

} // namespace

// [Device]

TEST_SUITE("Device") {
	TEST_CASE("Context creates successfully") {
		CHECK(Fixture().ctx != nullptr);
	}

	TEST_CASE("Underlying device is accessible and valid") {
		RHI::IRHIDevice *dev = &Ctx().GetDevice();
		CHECK(dev != nullptr);
	}

	TEST_CASE("WaitIdle does not crash") {
		CHECK_NOTHROW(Ctx().WaitIdle());
	}
}

// [Buffer]

TEST_SUITE("Buffer") {
	TEST_CASE("GPU_ONLY vertex buffer creates successfully") {
		RHI::BufferDesc desc{};
		desc.size = sizeof(float) * 12;
		desc.usage = RHI::BufferUsage::VertexBuffer | RHI::BufferUsage::TransferDst;
		desc.domain = RHI::MemoryDomain::GPU_ONLY;
		desc.debugName = "Test_VertexBuf";

		auto buf = Ctx().CreateBuffer(desc);
		CHECK(buf != nullptr);
	}

	TEST_CASE("CPU_TO_GPU uniform buffer creates successfully") {
		RHI::BufferDesc desc{};
		desc.size = 256;
		desc.usage = RHI::BufferUsage::UniformBuffer;
		desc.domain = RHI::MemoryDomain::CPU_TO_GPU;
		desc.debugName = "Test_UniformBuf";

		auto buf = Ctx().CreateBuffer(desc);
		CHECK(buf != nullptr);
	}

	TEST_CASE("CPU_ONLY staging buffer creates successfully") {
		RHI::BufferDesc desc{};
		desc.size = 1024;
		desc.usage = RHI::BufferUsage::TransferSrc;
		desc.domain = RHI::MemoryDomain::CPU_ONLY;
		desc.debugName = "Test_StagingBuf";

		auto buf = Ctx().CreateBuffer(desc);
		CHECK(buf != nullptr);
	}

	TEST_CASE("GPU_TO_CPU readback buffer creates successfully") {
		RHI::BufferDesc desc{};
		desc.size = 512;
		desc.usage = RHI::BufferUsage::TransferDst;
		desc.domain = RHI::MemoryDomain::GPU_TO_CPU;
		desc.debugName = "Test_ReadbackBuf";

		auto buf = Ctx().CreateBuffer(desc);
		CHECK(buf != nullptr);
	}

	TEST_CASE("Multiple buffers have distinct handles") {
		RHI::BufferDesc desc{};
		desc.size = 64;
		desc.usage = RHI::BufferUsage::UniformBuffer;
		desc.domain = RHI::MemoryDomain::CPU_TO_GPU;

		desc.debugName = "BufA";
		auto a = Ctx().CreateBuffer(desc);
		desc.debugName = "BufB";
		auto b = Ctx().CreateBuffer(desc);

		CHECK(a.get() != b.get());
	}

	TEST_CASE("Storage buffer creates successfully") {
		RHI::BufferDesc desc{};
		desc.size = 2048;
		desc.usage = RHI::BufferUsage::StorageBuffer | RHI::BufferUsage::TransferDst;
		desc.domain = RHI::MemoryDomain::GPU_ONLY;
		desc.debugName = "Test_StorageBuf";

		auto buf = Ctx().CreateBuffer(desc);
		CHECK(buf != nullptr);
	}
}

// [Texture]

TEST_SUITE("Texture") {
	TEST_CASE("RGBA8 sampled texture creates successfully") {
		RHI::TextureDesc desc{};
		desc.width = 256;
		desc.height = 256;
		desc.format = RHI::TextureFormat::RGBA8;
		desc.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst;
		desc.debugName = "Test_RGBA8";

		auto tex = Ctx().CreateTexture(desc);
		CHECK(tex != nullptr);
	}

	TEST_CASE("RGBA16F storage texture creates successfully") {
		RHI::TextureDesc desc{};
		desc.width = 512;
		desc.height = 512;
		desc.format = RHI::TextureFormat::RGBA16F;
		desc.usage = RHI::TextureUsage::Storage | RHI::TextureUsage::Sampled;
		desc.debugName = "Test_RGBA16F_Storage";

		auto tex = Ctx().CreateTexture(desc);
		CHECK(tex != nullptr);
	}

	TEST_CASE("Depth32 attachment texture creates successfully") {
		RHI::TextureDesc desc{};
		desc.width = 1280;
		desc.height = 720;
		desc.format = RHI::TextureFormat::Depth32;
		desc.usage = RHI::TextureUsage::DepthAttachment | RHI::TextureUsage::Sampled;
		desc.debugName = "Test_Depth";

		auto tex = Ctx().CreateTexture(desc);
		CHECK(tex != nullptr);
	}

	TEST_CASE("Color attachment texture creates successfully") {
		RHI::TextureDesc desc{};
		desc.width = 1280;
		desc.height = 720;
		desc.format = RHI::TextureFormat::RGBA8;
		desc.usage = RHI::TextureUsage::ColorAttachment | RHI::TextureUsage::Sampled;
		desc.debugName = "Test_ColorAttachment";

		auto tex = Ctx().CreateTexture(desc);
		CHECK(tex != nullptr);
	}

	TEST_CASE("Texture reports correct dimensions") {
		RHI::TextureDesc desc{};
		desc.width = 128;
		desc.height = 64;
		desc.format = RHI::TextureFormat::RGBA8;
		desc.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst;
		desc.debugName = "Test_DimCheck";

		auto tex = Ctx().CreateTexture(desc);
		REQUIRE(tex != nullptr);
		CHECK(tex->GetWidth() == 128u);
		CHECK(tex->GetHeight() == 64u);
	}

	TEST_CASE("Texture reports correct format") {
		RHI::TextureDesc desc{};
		desc.width = 64;
		desc.height = 64;
		desc.format = RHI::TextureFormat::RGBA32F;
		desc.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst;
		desc.debugName = "Test_FormatCheck";

		auto tex = Ctx().CreateTexture(desc);
		REQUIRE(tex != nullptr);
		CHECK(tex->GetFormat() == RHI::TextureFormat::RGBA32F);
	}

	TEST_CASE("Mipped texture creates successfully") {
		RHI::TextureDesc desc{};
		desc.width = 512;
		desc.height = 512;
		desc.mipLevels = 4;
		desc.format = RHI::TextureFormat::RGBA8;
		desc.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst | RHI::TextureUsage::TransferSrc;
		desc.debugName = "Test_Mipped";

		auto tex = Ctx().CreateTexture(desc);
		CHECK(tex != nullptr);
	}
}

// [Swapchain]

TEST_SUITE("Swapchain") {
	TEST_CASE("Swapchain creates successfully") {
		RHI::SwapchainDesc desc{};
		desc.width = 800;
		desc.height = 600;
		desc.format = RHI::TextureFormat::BGRA8;
		desc.imageCount = 2;
		desc.vsync = true;

		auto sc = Ctx().CreateSwapchain(desc);
		CHECK(sc != nullptr);
	}

	TEST_CASE("Swapchain reports at least one image") {
		RHI::SwapchainDesc desc{ .width = 800, .height = 600, .format = RHI::TextureFormat::BGRA8 };
		auto sc = Ctx().CreateSwapchain(desc);
		REQUIRE(sc != nullptr);
		CHECK(sc->GetImageCount() >= 1u);
	}

	TEST_CASE("Swapchain reports non-zero dimensions") {
		RHI::SwapchainDesc desc{ .width = 800, .height = 600, .format = RHI::TextureFormat::BGRA8 };
		auto sc = Ctx().CreateSwapchain(desc);
		REQUIRE(sc != nullptr);
		CHECK(sc->GetWidth() > 0u);
		CHECK(sc->GetHeight() > 0u);
	}

	TEST_CASE("Swapchain format is not None") {
		RHI::SwapchainDesc desc{ .width = 800, .height = 600, .format = RHI::TextureFormat::BGRA8 };
		auto sc = Ctx().CreateSwapchain(desc);
		REQUIRE(sc != nullptr);
		CHECK(sc->GetFormat() != RHI::TextureFormat::None);
	}
}

// [CommandList]

TEST_SUITE("CommandList") {
	TEST_CASE("Graphics command list creates successfully") {
		auto cmd = Ctx().CreateCommandList(RHI::CommandListType::Graphics, "Test_Graphics");
		CHECK(cmd != nullptr);
	}

	TEST_CASE("Compute command list creates successfully") {
		auto cmd = Ctx().CreateCommandList(RHI::CommandListType::Compute, "Test_Compute");
		CHECK(cmd != nullptr);
	}

	TEST_CASE("Transfer command list creates successfully") {
		auto cmd = Ctx().CreateCommandList(RHI::CommandListType::Transfer, "Test_Transfer");
		CHECK(cmd != nullptr);
	}

	TEST_CASE("Command list Begin/End cycle does not crash") {
		auto cmd = Ctx().CreateCommandList(RHI::CommandListType::Graphics, "Test_BeginEnd");
		REQUIRE(cmd != nullptr);
		CHECK_NOTHROW(cmd->Begin());
		CHECK_NOTHROW(cmd->End());
	}

	TEST_CASE("SubmitAndWait with empty command list does not crash") {
		auto cmd = Ctx().CreateCommandList(RHI::CommandListType::Graphics, "Test_SubmitWait");
		REQUIRE(cmd != nullptr);
		cmd->Begin();
		cmd->End();
		CHECK_NOTHROW(Ctx().SubmitAndWait(*cmd));
	}

	TEST_CASE("ExecuteImmediate does not crash") {
		CHECK_NOTHROW(Ctx().ExecuteImmediate(RHI::CommandListType::Transfer, [](GFX::GfxCommandList &) {}));
	}
}
