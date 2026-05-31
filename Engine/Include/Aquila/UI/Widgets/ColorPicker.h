#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/Popup.h"
#include "Aquila/UI/Widgets/Slider.h"
#include "Aquila/UI/Widgets/TextInput.h"
#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Aquila::UI::Core {

class ColorPicker : public View {
  public:
	class PickerArea;

	ColorPicker(GFX::GfxContext &ctx, vec4 color = vec4(1.f));

	[[nodiscard]] std::string_view GetTypeName() const override { return "ColorPicker"; }

	void SetColor(vec4 color);
	void SetOnChanged(Delegate<void(vec4)> callback);
	[[nodiscard]] vec4 GetColor() const { return m_Color; }

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnDrawSelf(Rendering::DrawList &drawList) override;

  private:
	enum class Mode { RGB, HSV, HEX };

	struct ChannelRow {
		View *row = nullptr;
		Label *label = nullptr;
		Slider *slider = nullptr;
		TextInput *input = nullptr;
	};

	void Init();
	void TogglePopup();
	void SetMode(Mode mode);

	void SyncAll();
	void SyncChannelDisplays();
	void SyncHexDisplay();
	void RebuildSVTexture();
	void RebuildAlphaTexture();
	void RebuildChannelTextures();
	void ApplyChannelValue(int idx, float rawValue);

	[[nodiscard]] static std::string FmtInt(int v);
	[[nodiscard]] static std::string FmtHex(vec4 color);
	[[nodiscard]] static bool ParseHex(const std::string &s, vec4 &out);

	GFX::GfxContext &m_Ctx;

	vec4 m_Color = vec4(1.f);
	float m_H = 0.f, m_S = 0.f, m_V = 1.f;
	Mode m_Mode = Mode::RGB;

	Ref<GFX::GfxTexture> m_SVTex;
	Ref<GFX::GfxTexture> m_HueTex;
	Ref<GFX::GfxTexture> m_AlphaTex;
	Ref<GFX::GfxTexture> m_ChTex[4];

	View *m_Swatch = nullptr;
	Popup *m_Popup = nullptr;
	View *m_Preview = nullptr;

	PickerArea *m_SVArea = nullptr;
	PickerArea *m_HueBar = nullptr;
	PickerArea *m_AlphaBar = nullptr;

	Button *m_RGBBtn = nullptr;
	Button *m_HSVBtn = nullptr;
	Button *m_HEXBtn = nullptr;

	ChannelRow m_Ch[4];

	View *m_HexRow = nullptr;
	TextInput *m_HexInput = nullptr;

	Delegate<void(vec4)> m_OnChanged;

	static constexpr float kSwatchH = 28.f;
	static constexpr float kPopupW = 272.f;
	static constexpr float kPad = 8.f;
	static constexpr float kSVH = 200.f;
	static constexpr float kBarH = 14.f;
	static constexpr uint32 kTexW = 256;
	static constexpr uint32 kSVTexH = 256;
	static constexpr uint32 kBarTexH = 8;
};

} // namespace Aquila::UI::Core
