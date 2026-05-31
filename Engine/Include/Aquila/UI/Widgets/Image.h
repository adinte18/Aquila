#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class Image : public View {
  public:
	Image() = default;
	Image(GFX::GfxTexture *texture, vec4 tint = vec4(1.f));

	[[nodiscard]] std::string_view GetTypeName() const override { return "Image"; }

	void SetTexture(GFX::GfxTexture *texture);
	void SetTint(vec4 tint);
	void SetUVRegion(vec2 uvMin, vec2 uvMax);

	[[nodiscard]] GFX::GfxTexture *GetTexture() const { return m_Texture; }
	[[nodiscard]] vec4 GetTint() const { return m_Tint; }
	[[nodiscard]] vec2 GetUVMin() const { return m_UVMin; }
	[[nodiscard]] vec2 GetUVMax() const { return m_UVMax; }

	// Returns the texture dimensions so Clay auto-sizes the widget when no CSS size is set.
	[[nodiscard]] vec2 GetIntrinsicSize() const override {
		if (m_Texture == nullptr) {
			return { -1.f, -1.f };
		}
		const vec2 texSize = { static_cast<float>(m_Texture->GetWidth()), static_cast<float>(m_Texture->GetHeight()) };
		const vec2 uvSpan = m_UVMax - m_UVMin;
		return texSize * uvSpan;
	}

	void OnDrawSelf(Rendering::DrawList &drawList) override;
	void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr) override;

  private:
	GFX::GfxTexture *m_Texture = nullptr;
	vec4 m_Tint = vec4(1.f);
	vec2 m_UVMin = { 0.f, 0.f };
	vec2 m_UVMax = { 1.f, 1.f };
};

} // namespace Aquila::UI::Core
