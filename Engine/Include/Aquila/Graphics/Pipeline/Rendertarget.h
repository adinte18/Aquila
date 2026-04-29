#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Defines.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::Graphics {

class RenderTarget {
  public:
	enum class Type : uint8 { COLOR_ONLY, DEPTH_ONLY, COLOR_DEPTH, GBUFFER, MRT };
	enum class Usage : uint8 { SCENE_VIEW, GAME_VIEW, SHADOW_MAP, POST_PROCESS, DEBUG_VIEW, GBUFFER_PASS };

	struct AttachmentSpec {
		RHI::TextureFormat format = RHI::TextureFormat::RGBA8;
		RHI::TextureUsage usage = RHI::TextureUsage::ColorAttachment;
		std::string debugName = "Attachment";
	};

	struct RenderTargetSpecification {
		uint32 width = 1280;
		uint32 height = 720;

		RHI::TextureFormat colorFormat = RHI::TextureFormat::RGBA8;
		RHI::TextureFormat depthFormat = RHI::TextureFormat::Depth32;

		RHI::SampleCount samples = RHI::SampleCount::x1;

		Type type = Type::COLOR_DEPTH;
		Usage usage = Usage::SCENE_VIEW;

		std::string debugName = "RenderTarget";

		std::vector<AttachmentSpec> colorAttachments;
	};

	class Builder {
	  public:
		Builder(const std::string &debugName) { m_Spec.debugName = debugName; }

		Builder &WithSize(uint32 width, uint32 height) {
			m_Spec.width = width;
			m_Spec.height = height;
			return *this;
		}
		Builder &WithColorFormat(RHI::TextureFormat format) {
			m_Spec.colorFormat = format;
			return *this;
		}
		Builder &WithDepthFormat(RHI::TextureFormat format) {
			m_Spec.depthFormat = format;
			return *this;
		}
		Builder &WithSamples(RHI::SampleCount samples) {
			m_Spec.samples = samples;
			return *this;
		}
		Builder &WithType(Type type) {
			m_Spec.type = type;
			return *this;
		}
		Builder &WithUsage(Usage usage) {
			m_Spec.usage = usage;
			return *this;
		}
		Builder &AddColorAttachment(RHI::TextureFormat format, const std::string &name,
									RHI::TextureUsage usage = RHI::TextureUsage::ColorAttachment) {
			m_Spec.colorAttachments.push_back({ format, usage, m_Spec.debugName + "_" + name });
			m_Spec.type = Type::MRT;
			return *this;
		}

		Ref<RenderTarget> Build() { return CreateRef<RenderTarget>(m_Spec); }

	  private:
		RenderTargetSpecification m_Spec;
	};

	explicit RenderTarget(RenderTargetSpecification spec) : m_Spec(std::move(spec)) {}
	~RenderTarget() = default;

	AQUILA_NONCOPYABLE(RenderTarget);
	AQUILA_NONMOVEABLE(RenderTarget);

	void Resize(uint32 width, uint32 height);

	[[nodiscard]] GFX::GfxTexture *GetColorAttachment(uint32 index = 0) const;
	[[nodiscard]] GFX::GfxTexture *GetDepthAttachment() const;
	[[nodiscard]] const std::vector<Ref<GFX::GfxTexture>> &GetColorAttachments() const { return m_ColorAttachments; }

	[[nodiscard]] RHI::TextureFormat GetColorFormat(uint32 index = 0) const {
		if (index < m_Spec.colorAttachments.size()) {
			return m_Spec.colorAttachments[index].format;
		}
		return m_Spec.colorFormat;
	}
	[[nodiscard]] RHI::TextureFormat GetDepthFormat() const { return m_Spec.depthFormat; }
	[[nodiscard]] uint32 GetWidth() const { return m_Spec.width; }
	[[nodiscard]] uint32 GetHeight() const { return m_Spec.height; }
	[[nodiscard]] RHI::SampleCount GetSamples() const { return m_Spec.samples; }
	[[nodiscard]] bool IsFirstUse() const { return m_FirstUse; }
	[[nodiscard]] bool IsValid() const;
	void MarkUsed() { m_FirstUse = false; }

	[[nodiscard]] const RenderTargetSpecification &GetSpec() const { return m_Spec; }

  private:
	void Create();
	void CreateSingleColorAttachment();
	void CreateMRTAttachments();
	void CreateDepthAttachment();

	RenderTargetSpecification m_Spec;
	bool m_FirstUse = true;

	Ref<GFX::GfxTexture> m_ColorAttachment;
	Ref<GFX::GfxTexture> m_DepthAttachment;
	std::vector<Ref<GFX::GfxTexture>> m_ColorAttachments;
};

} // namespace Aquila::Graphics
#endif
