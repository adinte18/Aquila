#ifndef AQUILA_RHI_TYPES_H
#define AQUILA_RHI_TYPES_H

#include "Aquila/Foundation/Defines.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::RHI {

class IRHIDescriptorSetLayout;
class IRHITexture;
class IRHISwapchain;

// NOTES:
// USE GPU_ONLY FOR GPU LOCAL BUFFERS - vertex, index, storage
// USE CPU_TO_GPU FOR SEQUENTIAL WRITES - uniform buffers or staging buffers
// USE GPU_TO_CPU FOR RANDOM ACCESS - readbacks
enum class MemoryDomain : uint8 { GPU_ONLY, CPU_ONLY, GPU_TO_CPU, CPU_TO_GPU };

enum class TextureFormat : uint8 {
	None = 0,
	RGBA8,
	RGBA8_SRGB,
	RGBA16F,
	RGBA32F,
	RGBA32U,
	RGB8,
	RGB16F,
	RGB32F,
	RG8,
	RG16F,
	RG32F,
	R8,
	R16F,
	R32F,
	R32UI,
	BGRA8,
	BGRA8_SRGB,
	Depth16,
	Depth32,
	Depth24Stencil8,
	Depth32Stencil8,
};

// Bitmask so usages can be combined
enum class TextureUsage : uint8 {
	None = 0,
	ColorAttachment = BIT(0),
	DepthAttachment = BIT(1),
	Sampled = BIT(2),
	Storage = BIT(3),
	TransferSrc = BIT(4),
	TransferDst = BIT(5),
	InputAttachment = BIT(6),
};

AQUILA_FORCE_INLINE TextureUsage operator|(TextureUsage a, TextureUsage b) {
	return static_cast<TextureUsage>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
AQUILA_FORCE_INLINE TextureUsage operator&(TextureUsage a, TextureUsage b) {
	return static_cast<TextureUsage>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

enum class SampleCount : uint8 { x1, x2, x4, x8, x16, x32, x64 };

enum class ShaderStageFlags : uint8 {
	None = 0,
	Vertex = 1 << 0,
	Fragment = 1 << 1,
	Compute = 1 << 2,
	Geometry = 1 << 3,
	All = Vertex | Fragment | Compute | Geometry,
};

AQUILA_FORCE_INLINE ShaderStageFlags operator|(ShaderStageFlags a, ShaderStageFlags b) {
	return static_cast<ShaderStageFlags>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
AQUILA_FORCE_INLINE ShaderStageFlags operator&(ShaderStageFlags a, ShaderStageFlags b) {
	return static_cast<ShaderStageFlags>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

enum class FilterMode : uint8 { Nearest, Linear };
enum class MipmapMode : uint8 { Nearest, Linear };

enum class AddressMode : uint8 {
	Repeat,
	MirroredRepeat,
	ClampToEdge,
	ClampToBorder,
};

enum class BorderColor : uint8 {
	TransparentBlack,
	OpaqueBlack,
	OpaqueWhite,
};

enum class CompareOp : uint8 {
	Never,
	Less,
	Equal,
	LessEqual,
	Greater,
	NotEqual,
	GreaterEqual,
	Always,
};

enum class CommandListType : uint8 { Graphics, Compute, Transfer };

enum class BufferUsage : uint32 {
	None = 0,
	VertexBuffer = BIT(0),
	IndexBuffer = BIT(1),
	UniformBuffer = BIT(2),
	StorageBuffer = BIT(3),
	TransferSrc = BIT(4),
	TransferDst = BIT(5),
	IndirectBuffer = BIT(6),
};

AQUILA_FORCE_INLINE BufferUsage operator|(BufferUsage a, BufferUsage b) {
	return static_cast<BufferUsage>(static_cast<uint32>(a) | static_cast<uint32>(b));
}
AQUILA_FORCE_INLINE BufferUsage operator&(BufferUsage a, BufferUsage b) {
	return static_cast<BufferUsage>(static_cast<uint32>(a) & static_cast<uint32>(b));
}

enum class IndexFormat : uint8 { UInt16, UInt32 };

enum class TextureViewType : uint8 {
	Tex1D,
	Tex2D,
	Tex3D,
	Cube,
	Tex1DArray,
	Tex2DArray,
	CubeArray,
};

enum class ComponentSwizzle : uint8 { Identity, Zero, One, R, G, B, A };

struct ComponentMapping {
	ComponentSwizzle r = ComponentSwizzle::Identity;
	ComponentSwizzle g = ComponentSwizzle::Identity;
	ComponentSwizzle b = ComponentSwizzle::Identity;
	ComponentSwizzle a = ComponentSwizzle::Identity;

	bool operator==(const ComponentMapping &) const = default;
};

struct SamplerDesc {
	FilterMode magFilter = FilterMode::Linear;
	FilterMode minFilter = FilterMode::Linear;
	MipmapMode mipmapMode = MipmapMode::Nearest;
	AddressMode addressU = AddressMode::ClampToEdge;
	AddressMode addressV = AddressMode::ClampToEdge;
	AddressMode addressW = AddressMode::ClampToEdge;
	BorderColor borderColor = BorderColor::TransparentBlack;
	float minLod = 0.0f;
	float maxLod = 0.0f;
	float mipLodBias = 0.0f;
	bool anisotropy = false;
	bool compareEnable = false;
	CompareOp compareOp = CompareOp::Always;

	bool operator==(const SamplerDesc &) const = default;

	static SamplerDesc Texture2D(float maxLod = 1000.0f) {
		SamplerDesc d{};
		d.addressU = AddressMode::Repeat;
		d.addressV = AddressMode::Repeat;
		d.addressW = AddressMode::Repeat;
		d.mipmapMode = MipmapMode::Linear;
		d.maxLod = maxLod;
		d.anisotropy = true;
		return d;
	}

	static SamplerDesc RenderTarget() {
		SamplerDesc d{};
		d.mipmapMode = MipmapMode::Linear;
		return d;
	}

	static SamplerDesc ShadowMap() {
		SamplerDesc d{};
		d.addressU = AddressMode::ClampToBorder;
		d.addressV = AddressMode::ClampToBorder;
		d.addressW = AddressMode::ClampToBorder;
		d.borderColor = BorderColor::OpaqueWhite;
		d.compareEnable = true;
		d.compareOp = CompareOp::LessEqual;
		return d;
	}

	static SamplerDesc PointSample() {
		SamplerDesc d{};
		d.magFilter = FilterMode::Nearest;
		d.minFilter = FilterMode::Nearest;
		d.mipmapMode = MipmapMode::Nearest;
		d.addressU = AddressMode::ClampToEdge;
		d.addressV = AddressMode::ClampToEdge;
		d.addressW = AddressMode::ClampToEdge;
		d.anisotropy = false;
		return d;
	}

	static SamplerDesc FontAtlas() {
		SamplerDesc d{};
		d.magFilter = FilterMode::Linear;
		d.minFilter = FilterMode::Linear;

		d.mipmapMode = MipmapMode::Nearest;

		d.addressU = AddressMode::ClampToEdge;
		d.addressV = AddressMode::ClampToEdge;
		d.addressW = AddressMode::ClampToEdge;

		d.anisotropy = false;
		d.minLod = 0.0f;
		d.maxLod = 0.0f;

		return d;
	}
};

struct SamplerDescHash {
	size_t operator()(const SamplerDesc &d) const {
		size_t h = 0;
		auto combine = [&](auto v) {
			h ^= std::hash<uint32>{}(static_cast<uint32>(v)) + 0x9e3779b9 + (h << 6) + (h >> 2);
		};
		combine(d.magFilter);
		combine(d.minFilter);
		combine(d.mipmapMode);
		combine(d.addressU);
		combine(d.addressV);
		combine(d.addressW);
		combine(d.borderColor);
		combine(d.compareEnable);
		combine(d.compareOp);
		return h;
	}
};

struct BufferDesc {
	uint64 size = 0;
	BufferUsage usage = BufferUsage::None;
	MemoryDomain domain = MemoryDomain::GPU_ONLY;
	uint32 instanceCount = 1;
	uint64 minAlignment = 0;
	std::string debugName;
};

struct TextureDesc {
	uint32 width = 1;
	uint32 height = 1;
	uint32 depth = 1;
	uint32 mipLevels = 1;
	uint32 arrayLayers = 1;
	TextureFormat format = TextureFormat::RGBA8;
	TextureUsage usage = TextureUsage::Sampled;
	SampleCount samples = SampleCount::x1;
	TextureViewType viewType = TextureViewType::Tex2D;
	ComponentMapping swizzle = {};
	SamplerDesc sampler = SamplerDesc::Texture2D();
	std::string debugName = "Texture";
};

struct SwapchainDesc {
	uint32 width = 0;
	uint32 height = 0;
	TextureFormat format = TextureFormat::BGRA8;
	uint32 imageCount = 2;
	bool vsync = true;
};

enum class PrimitiveTopology : uint8 {
	TriangleList,
	TriangleStrip,
	TriangleFan,
	LineList,
	LineStrip,
	PointList,
};

enum class CullMode : uint8 { None, Front, Back };
enum class FillMode : uint8 { Solid, Wireframe };
enum class FrontFace : uint8 { Clockwise, CounterClockwise };

enum class BlendFactor : uint8 {
	Zero,
	One,
	SrcColor,
	OneMinusSrcColor,
	DstColor,
	OneMinusDstColor,
	SrcAlpha,
	OneMinusSrcAlpha,
	DstAlpha,
	OneMinusDstAlpha,
};

enum class BlendOp : uint8 { Add, Subtract, ReverseSubtract, Min, Max };

enum class DescriptorType : uint8 {
	UniformBuffer,
	StorageBuffer,
	CombinedImageSampler,
	StorageImage,
	InputAttachment,
};

struct VertexAttributeDesc {
	uint32 location;
	uint32 binding;
	TextureFormat format;
	uint32 offset;
};

struct VertexBindingDesc {
	uint32 stride;
	std::vector<VertexAttributeDesc> attributes;
};

struct ShaderStageDesc {
	ShaderStageFlags stage = ShaderStageFlags::Vertex;
	std::vector<uint32> spirv;
	std::string entryPoint = "main";
};

struct PushConstantRange {
	ShaderStageFlags stages = ShaderStageFlags::Vertex;
	uint32 offset = 0;
	uint32 size = 0;
};

struct BlendAttachmentDesc {
	bool enable = false;
	BlendFactor srcColor = BlendFactor::SrcAlpha;
	BlendFactor dstColor = BlendFactor::OneMinusSrcAlpha;
	BlendOp colorOp = BlendOp::Add;
	BlendFactor srcAlpha = BlendFactor::One;
	BlendFactor dstAlpha = BlendFactor::Zero;
	BlendOp alphaOp = BlendOp::Add;
};

struct RasterStateDesc {
	CullMode cullMode = CullMode::Back;
	FillMode fillMode = FillMode::Solid;
	FrontFace frontFace = FrontFace::CounterClockwise;
	bool depthClamp = false;
	float lineWidth = 1.0f;
};

struct DepthStencilStateDesc {
	bool depthTest = true;
	bool depthWrite = true;
	CompareOp depthCompare = CompareOp::Less;
	bool stencilTest = false;
};

struct GraphicsPipelineDesc {
	ShaderStageDesc vertexShader;
	ShaderStageDesc fragmentShader;
	PrimitiveTopology topology = PrimitiveTopology::TriangleList;
	RasterStateDesc raster;
	DepthStencilStateDesc depthStencil;
	std::vector<BlendAttachmentDesc> blendAttachments = { {} };
	std::vector<TextureFormat> colorFormats;
	TextureFormat depthFormat = TextureFormat::Depth32;
	SampleCount sampleCount = SampleCount::x1;
	std::vector<IRHIDescriptorSetLayout *> setLayouts;
	std::vector<PushConstantRange> pushConstants;
	std::optional<VertexBindingDesc> customVertexLayout;
	bool noVertexInput = false; // true for shader-only draws (SV_VertexID, no VB)
	std::string debugName;
};

struct ComputePipelineDesc {
	ShaderStageDesc computeShader;
	std::vector<IRHIDescriptorSetLayout *> setLayouts;
	std::vector<PushConstantRange> pushConstants;
	std::string debugName;
};

struct DescriptorBinding {
	uint32 binding = 0;
	DescriptorType type = DescriptorType::UniformBuffer;
	ShaderStageFlags stages = ShaderStageFlags::Vertex;
	uint32 count = 1;
};

struct DescriptorSetLayoutDesc {
	std::vector<DescriptorBinding> bindings;
};

enum class AttachmentLoadOp : uint8 { Load, Clear, DontCare };
enum class AttachmentStoreOp : uint8 { Store, DontCare };

struct RenderPassColorAttachmentDesc {
	IRHITexture *texture = nullptr;		   // null = use swapchain image
	IRHITexture *resolveTexture = nullptr; // MSAA resolve target
	vec4 clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
	AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
	uint32 mipLevel = 0;   // target mip level (for rendering into a mip)
	uint32 arrayLayer = 0; // target array layer / cubemap face
};

struct RenderPassDepthAttachmentDesc {
	IRHITexture *texture = nullptr; // null = use swapchain depth
	float clearDepth = 1.0f;
	uint8 clearStencil = 0;
	AttachmentLoadOp depthLoadOp = AttachmentLoadOp::Clear;
	AttachmentStoreOp depthStoreOp = AttachmentStoreOp::DontCare;
	AttachmentLoadOp stencilLoadOp = AttachmentLoadOp::DontCare;
	AttachmentStoreOp stencilStoreOp = AttachmentStoreOp::DontCare;
	bool readOnly = false; // depth test but no write (read-only depth attachment)
};

struct RenderPassDesc {
	std::vector<RenderPassColorAttachmentDesc> colorAttachments;
	std::optional<RenderPassDepthAttachmentDesc> depthAttachment;
	bool useSwapchain = false;
	uint32 width = 0;
	uint32 height = 0;

	std::string debugName = "RenderPass";

	// When true, VulkanRenderPass::Begin/End will NOT emit its built-in pre/post
	// barriers.  Set by the RenderGraph compiler, which handles all transitions
	// through its own barrier system.
	bool externalBarriers = false;
};

enum class ResourceState : uint16 {
	Undefined = 0,
	ColorAttachmentRead = 1 << 0,
	ColorAttachmentWrite = 1 << 1,
	DepthStencilRead = 1 << 2,
	DepthStencilWrite = 1 << 3,
	ShaderRead = 1 << 4,   // sampled / SRV
	StorageRead = 1 << 5,  // UAV read
	StorageWrite = 1 << 6, // UAV write
	UniformRead = 1 << 7,
	TransferSrc = 1 << 8,
	TransferDst = 1 << 9,
	IndirectArgument = 1 << 10,
	IndexBuffer = 1 << 11,
	VertexBuffer = 1 << 12,
	Present = 1 << 13,
	ColorAttachment = ColorAttachmentRead | ColorAttachmentWrite,
	DepthRead = DepthStencilRead,
	DepthWrite = DepthStencilRead | DepthStencilWrite,
	UnorderedAccess = StorageRead | StorageWrite,

};

} // namespace Aquila::RHI
#endif
