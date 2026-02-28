#ifndef RENDERTARGET_H
#define RENDERTARGET_H

#include <utility>

#include "Aquila/Graphics/Core/Device.h"
#include "Aquila/Graphics/Resources/Texture2D.h"

namespace Aquila::Graphics::RenderingPipeline {

class RenderTarget {
  public:
	enum class Type : uint8 { COLOR_ONLY, DEPTH_ONLY, COLOR_DEPTH, GBUFFER, MRT };

	enum class Usage : uint8 { SCENE_VIEW, GAME_VIEW, SHADOW_MAP, POST_PROCESS, DEBUG_VIEW, GBUFFER_PASS };

	struct AttachmentSpec {
		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		std::string debugName = "Attachment";
	};

	struct RenderTargetSpecification {
		uint32 width = 1280;
		uint32 height = 720;

		VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

		VkImageUsageFlags colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

		Type type = Type::COLOR_DEPTH;
		Usage usage = Usage::SCENE_VIEW;

		std::string debugName = "RenderTarget";
		bool isCubemap = false;

		std::vector<AttachmentSpec> colorAttachments;
		AttachmentSpec depthAttachment;
	};

	class Builder {
	  public:
		Builder(Device &device, const std::string &debugName) : m_Device(device) { m_Spec.debugName = debugName; }

		Builder &WithSize(uint32 width, uint32 height) {
			m_Spec.width = width;
			m_Spec.height = height;
			return *this;
		}

		Builder &WithColorFormat(VkFormat format) {
			m_Spec.colorFormat = format;
			return *this;
		}

		Builder &WithDepthFormat(VkFormat format) {
			m_Spec.depthAttachment.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			m_Spec.depthAttachment.format = format;
			m_Spec.depthFormat = format;
			return *this;
		}

		Builder &WithSamples(VkSampleCountFlagBits samples) {
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

		Builder &WithColorUsage(VkImageUsageFlags usage) {
			m_Spec.colorUsage = usage;
			return *this;
		}

		Builder &WithDepthUsage(VkImageUsageFlags usage) {
			m_Spec.depthUsage = usage;
			return *this;
		}

		Builder &AddColorAttachment(VkFormat format, VkImageUsageFlags usage, const std::string &name) {
			AttachmentSpec spec;
			spec.format = format;
			spec.usage = usage;
			spec.debugName = m_Spec.debugName + "_" + name;
			m_Spec.colorAttachments.push_back(spec);
			m_Spec.type = Type::MRT;
			return *this;
		}

		Builder &AddColorAttachment(const AttachmentSpec &spec) {
			m_Spec.colorAttachments.push_back(spec);
			m_Spec.type = Type::MRT;
			return *this;
		}

		Builder &AsCubemap(uint32 width, uint32 height, VkFormat format) {
			m_Spec.width = width;
			m_Spec.height = height;
			m_Spec.colorFormat = format;
			m_Spec.isCubemap = true;
			// m_CreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			// m_ArrayLayers = 6;
			return *this;
		}

		Ref<RenderTarget> Build() { return CreateRef<RenderTarget>(m_Device, m_Spec); }

	  private:
		Device &m_Device;
		RenderTargetSpecification m_Spec;
	};

	RenderTarget(const RenderTarget &) = default;
	RenderTarget(RenderTarget &&) = delete;
	RenderTarget &operator=(const RenderTarget &) = delete;
	RenderTarget &operator=(RenderTarget &&) = delete;
	RenderTarget(Device &device, RenderTargetSpecification spec) : m_Device(device), m_Spec(std::move(spec)) {
		Create();
	}

	~RenderTarget() { Destroy(); }

	void Resize(uint32 width, uint32 height) {
		if (m_Spec.width == width && m_Spec.height == height) {
			return;
		}

		QueueForDeletion();

		m_Spec.width = width;
		m_Spec.height = height;

		Create();
	}

	void QueueForDeletion() { Destroy(); }

	void Destroy() {
		if (m_ColorAttachment) {
			m_ColorAttachment->Destroy();
			m_ColorAttachment.reset();
		}

		for (auto &attachment : m_ColorAttachments) {
			if (attachment) {
				attachment->Destroy();
			}
		}
		m_ColorAttachments.clear();

		if (m_DepthAttachment) {
			m_DepthAttachment->Destroy();
			m_DepthAttachment.reset();
		}
	}

	Ref<Resources::Texture2D> GetColorAttachment(uint32 index = 0) const {
		if (m_Spec.type == Type::MRT || m_Spec.type == Type::GBUFFER) {
			return index < m_ColorAttachments.size() ? m_ColorAttachments[index] : nullptr;
		}
		return index == 0 ? m_ColorAttachment : nullptr;
	}

	Ref<Resources::Texture2D> GetDepthAttachment() const { return m_DepthAttachment; }

	[[nodiscard]] const std::vector<Ref<Resources::Texture2D>> &GetColorAttachments() const {
		return m_ColorAttachments;
	}

	[[nodiscard]] uint32 GetColorAttachmentCount() const {
		return m_Spec.type == Type::MRT ? static_cast<uint32>(m_ColorAttachments.size()) : (m_ColorAttachment ? 1 : 0);
	}

	[[nodiscard]] VkImageView GetColorImageView(uint32 index = 0) const {
		auto attachment = GetColorAttachment(index);
		return attachment ? attachment->GetImageView() : VK_NULL_HANDLE;
	}

	[[nodiscard]] std::vector<VkImage> GetColorImages() const {
		std::vector<VkImage> result;

		if (m_Spec.type == Type::MRT || m_Spec.type == Type::GBUFFER) {
			// MRT: use the vector
			for (const auto &attachment : m_ColorAttachments) {
				if (attachment) {
					result.push_back(attachment->GetImage());
				}
			}
		} else if (m_ColorAttachment) {
			// Single attachment: use the singular member
			result.push_back(m_ColorAttachment->GetImage());
		}

		return result;
	}

	[[nodiscard]] VkImage GetDepthImage() const {
		auto attachment = GetDepthAttachment();
		return attachment->GetImage();
	}

	[[nodiscard]] VkImageView GetDepthImageView() const {
		return m_DepthAttachment ? m_DepthAttachment->GetImageView() : VK_NULL_HANDLE;
	}

	[[nodiscard]] std::vector<VkImageView> GetAllColorImageViews() const {
		std::vector<VkImageView> views;
		if (m_Spec.type == Type::MRT || m_Spec.type == Type::GBUFFER) {
			views.reserve(m_ColorAttachments.size());
			for (const auto &attachment : m_ColorAttachments) {
				if (attachment) {
					views.push_back(attachment->GetImageView());
				}
			}
		} else if (m_ColorAttachment) {
			views.push_back(m_ColorAttachment->GetImageView());
		}
		return views;
	}

	const RenderTargetSpecification &GetSpec() const { return m_Spec; }

	bool IsValid() const {
		switch (m_Spec.type) {
		case Type::COLOR_ONLY:
			return m_ColorAttachment != nullptr;
		case Type::DEPTH_ONLY:
			return m_DepthAttachment != nullptr;
		case Type::COLOR_DEPTH:
			return m_ColorAttachment != nullptr && m_DepthAttachment != nullptr;
		case Type::MRT:
		case Type::GBUFFER:
			return !m_ColorAttachments.empty();
		default:
			return false;
		}
	}

	bool IsFirstUse() const { return m_FirstUse; }
	void MarkUsed() { m_FirstUse = false; }

	VkFormat GetColorFormat() const { return m_Spec.colorFormat; }
	VkFormat GetDepthFormat() const { return m_Spec.depthFormat; }

	VkFormat GetColorFormat(uint32 index) const {
		if (index < m_Spec.colorAttachments.size()) {
			return m_Spec.colorAttachments[index].format;
		}
		return m_Spec.colorFormat;
	}

	std::vector<VkFormat> GetColorFormats() const {
		std::vector<VkFormat> formats;
		if (m_Spec.type == Type::MRT || m_Spec.type == Type::GBUFFER) {
			for (const auto &attachment : m_Spec.colorAttachments) {
				formats.push_back(attachment.format);
			}
		} else {
			formats.push_back(m_Spec.colorFormat);
		}
		return formats;
	}

  private:
	void Create() {
		m_FirstUse = true;
		if (m_Spec.type == Type::MRT || m_Spec.type == Type::GBUFFER) {
			CreateMRTAttachments();
		} else {
			CreateSingleColorAttachment();
		}

		if (m_Spec.type == Type::COLOR_ONLY || m_Spec.type == Type::COLOR_DEPTH) {
			AQUILA_ASSERT(m_ColorAttachment != nullptr, "Color attachment creation failed!");
		}

		if (m_Spec.type == Type::DEPTH_ONLY || m_Spec.type == Type::COLOR_DEPTH || m_Spec.type == Type::MRT ||
			m_Spec.type == Type::GBUFFER) {
			CreateDepthAttachment();
		}
	}

	void CreateMRTAttachments() {
		m_ColorAttachments.clear();
		m_ColorAttachments.reserve(m_Spec.colorAttachments.size());

		for (const auto &attachmentSpec : m_Spec.colorAttachments) {
			auto attachment = Resources::Texture2D::Builder(m_Device, attachmentSpec.debugName)
								  .AsRenderTarget(m_Spec.width, m_Spec.height, attachmentSpec.format,
												  attachmentSpec.usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
								  .WithSamples(m_Spec.samples)
								  .Build();
			m_ColorAttachments.push_back(attachment);
		}
	}

	void CreateSingleColorAttachment() {
		if (m_Spec.type == Type::COLOR_ONLY || m_Spec.type == Type::COLOR_DEPTH) {
			if (m_Spec.isCubemap) {
				// TODO: do i need that?
			} else {
				AQUILA_LOG_INFO("Creating single color attachment: {}", m_Spec.debugName);
				m_ColorAttachment = Resources::Texture2D::Builder(m_Device, m_Spec.debugName + "_Color")
										.AsRenderTarget(m_Spec.width, m_Spec.height, m_Spec.colorFormat,
														m_Spec.colorUsage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
										.WithSamples(m_Spec.samples)
										.Build();

				if (!m_ColorAttachment) {
					AQUILA_LOG_CRITICAL("FAILED TO CREATE COLOR ATTACHMENT: {}", m_Spec.debugName);
				} else {
					AQUILA_LOG_INFO("Successfully created color attachment: {}", m_Spec.debugName);
				}
			}
		}
	}
	void CreateDepthAttachment() {
		m_DepthAttachment = Resources::Texture2D::Builder(m_Device, m_Spec.debugName + "_Depth")
								.AsRenderTarget(m_Spec.width, m_Spec.height, m_Spec.depthFormat, m_Spec.depthUsage)
								.Build();
	}

	Device &m_Device;
	RenderTargetSpecification m_Spec;
	bool m_FirstUse = true;

	Ref<Resources::Texture2D> m_ColorAttachment;
	Ref<Resources::Texture2D> m_DepthAttachment;
	std::vector<Ref<Resources::Texture2D>> m_ColorAttachments;
};

} // namespace Aquila::Graphics::RenderingPipeline

#endif
