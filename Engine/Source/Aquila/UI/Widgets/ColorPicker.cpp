#include "Aquila/UI/Widgets/ColorPicker.h"
#include "Aquila/UI/Rendering/DrawCmd.h"
#include "Aquila/RHI/Backend/RHITypes.h"

namespace Aquila::UI::Core {

static vec3 HsvToRgb(float h, float s, float v) {
	if (s < 1e-6f) {
		return { v, v, v };
	}
	h = std::fmod(h, 1.f) * 6.f;
	const int i = static_cast<int>(h);
	const float f = h - static_cast<float>(i);
	const float p = v * (1.f - s);
	const float q = v * (1.f - s * f);
	const float t = v * (1.f - s * (1.f - f));
	switch (i % 6) {
	case 0:
		return { v, t, p };
	case 1:
		return { q, v, p };
	case 2:
		return { p, v, t };
	case 3:
		return { p, q, v };
	case 4:
		return { t, p, v };
	default:
		return { v, p, q };
	}
}

static vec3 RgbToHsv(float r, float g, float b) {
	const float mx = std::max({ r, g, b });
	const float mn = std::min({ r, g, b });
	const float d = mx - mn;
	float h = 0.f;
	const float s = (mx > 1e-6f) ? d / mx : 0.f;
	if (d > 1e-6f) {
		if (mx == r) {
			h = (g - b) / d / 6.f + (g < b ? 1.f : 0.f);
		} else if (mx == g) {
			h = (b - r) / d / 6.f + 1.f / 3.f;
		} else {
			h = (r - g) / d / 6.f + 2.f / 3.f;
		}
	}
	return { h, s, mx };
}

class ColorPicker::PickerArea : public View {
  public:
	GFX::GfxTexture *m_Tex = nullptr;
	vec2 m_Indicator = {};
	bool m_Is1D = false;
	Delegate<void(vec2)> m_OnPick;

	PickerArea() { SetInputLeaf(true); }

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override {
		View::OnMousePress(btn, pos);
		if (btn == Platform::MouseButton::Left) {
			Pick(pos);
		}
	}
	void OnMouseMove(vec2 pos) override {
		if (m_IsPressed) {
			Pick(pos);
		}
	}

	void OnDrawSelf(Rendering::DrawList &dl) override {
		View::OnDrawSelf(dl);
		const Rect r = { GetAbsolutePosition(), GetLayoutRect().size };
		const int32 z = GetStackingZ() * 4;

		if (m_Tex) {
			dl.DrawImage(r, m_Tex, vec4(1.f), vec2(0.f), vec2(1.f), z + 1);
		}

		if (m_Is1D) {
			const float ix = r.position.x + m_Indicator.x * r.size.x;
			const Rect ind = { .position = { ix - 3.f, r.position.y - 2.f }, .size = { 6.f, r.size.y + 4.f } };
			dl.DrawRect(ind, vec4(1.f), vec4(3.f), 1.5f, vec4(0.1f, 0.1f, 0.1f, 0.9f), z + 2);
		} else {
			const vec2 ic = r.position + m_Indicator * r.size;
			const float rd = 6.f;
			const Rect ind = { .position = { ic.x - rd, ic.y - rd }, .size = { rd * 2.f, rd * 2.f } };
			dl.DrawRect(ind, vec4(0.f, 0.f, 0.f, 0.f), vec4(rd), 2.f, vec4(0.f, 0.f, 0.f, 0.9f), z + 2);
			const float ri = 4.f;
			const Rect i2 = { .position = { ic.x - ri, ic.y - ri }, .size = { ri * 2.f, ri * 2.f } };
			dl.DrawRect(i2, vec4(0.f), vec4(ri), 1.5f, vec4(1.f, 1.f, 1.f, 0.85f), z + 3);
		}
	}

  private:
	void Pick(vec2 absPos) {
		const Rect r = GetAbsoluteRect();
		vec2 n = (absPos - r.position) / glm::max(r.size, vec2(1.f));
		n = glm::clamp(n, vec2(0.f), vec2(1.f));
		if (m_Is1D) {
			n.y = 0.f;
		}
		m_Indicator = n;
		QueueRedraw();
		if (m_OnPick) {
			m_OnPick(n);
		}
	}
};

static Ref<GFX::GfxTexture> MakeTex(GFX::GfxContext &ctx, uint32 w, uint32 h, const std::vector<uint8> &px,
									const char *name) {
	RHI::TextureDesc d;
	d.width = w;
	d.height = h;
	d.format = RHI::TextureFormat::RGBA8;
	d.usage = RHI::TextureUsage::Sampled | RHI::TextureUsage::TransferDst;
	d.debugName = name;
	auto tex = ctx.CreateTexture(d);
	ctx.UploadTextureData(*tex, px.data(), static_cast<uint64>(px.size()));
	return tex;
}

static std::vector<uint8> GenSV(float hue, uint32 w, uint32 h) {
	std::vector<uint8> px(w * h * 4);
	for (uint32 y = 0; y < h; ++y) {
		const float v = 1.f - y / static_cast<float>(h - 1);
		for (uint32 x = 0; x < w; ++x) {
			const float s = x / static_cast<float>(w - 1);
			const vec3 rgb = HsvToRgb(hue, s, v);
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			px[i + 0] = uint8(rgb.r * 255.f);
			px[i + 1] = uint8(rgb.g * 255.f);
			px[i + 2] = uint8(rgb.b * 255.f);
			px[i + 3] = 255;
		}
	}
	return px;
}

static std::vector<uint8> GenHue(uint32 w, uint32 h) {
	std::vector<uint8> px(w * h * 4);
	for (uint32 y = 0; y < h; ++y) {
		for (uint32 x = 0; x < w; ++x) {
			const vec3 rgb = HsvToRgb(x / static_cast<float>(w - 1), 1.f, 1.f);
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			px[i + 0] = uint8(rgb.r * 255.f);
			px[i + 1] = uint8(rgb.g * 255.f);
			px[i + 2] = uint8(rgb.b * 255.f);
			px[i + 3] = 255;
		}
	}
	return px;
}

static std::vector<uint8> GenChannelGrad(vec4 color, float h, float s, float v, int ch, bool isHSV, uint32 w,
										 uint32 texH) {
	std::vector<uint8> px(w * texH * 4);
	for (uint32 x = 0; x < w; ++x) {
		const float t = x / static_cast<float>(w - 1);
		for (uint32 y = 0; y < texH; ++y) {
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			if (ch == 3) {
				const float bg = ((x / 6) + (y / 6)) % 2 == 0 ? 0.75f : 0.5f;
				px[i + 0] = uint8((color.r * t + bg * (1.f - t)) * 255.f);
				px[i + 1] = uint8((color.g * t + bg * (1.f - t)) * 255.f);
				px[i + 2] = uint8((color.b * t + bg * (1.f - t)) * 255.f);
				px[i + 3] = 255;
			} else if (!isHSV) {
				float cr = color.r, cg = color.g, cb = color.b;
				switch (ch) {
				case 0:
					cr = t;
					break;
				case 1:
					cg = t;
					break;
				case 2:
					cb = t;
					break;
				}
				px[i + 0] = uint8(cr * 255.f);
				px[i + 1] = uint8(cg * 255.f);
				px[i + 2] = uint8(cb * 255.f);
				px[i + 3] = 255;
			} else {
				float ch_h = h, ch_s = s, ch_v = v;
				switch (ch) {
				case 0:
					ch_h = t;
					break;
				case 1:
					ch_s = t;
					break;
				case 2:
					ch_v = t;
					break;
				}
				const vec3 rgb = HsvToRgb(ch_h, ch_s, ch_v);
				px[i + 0] = uint8(rgb.r * 255.f);
				px[i + 1] = uint8(rgb.g * 255.f);
				px[i + 2] = uint8(rgb.b * 255.f);
				px[i + 3] = 255;
			}
		}
	}
	return px;
}

static std::vector<uint8> GenAlpha(vec3 rgb, uint32 w, uint32 h) {
	std::vector<uint8> px(w * h * 4);
	for (uint32 y = 0; y < h; ++y) {
		for (uint32 x = 0; x < w; ++x) {
			const float a = x / static_cast<float>(w - 1);
			const float bg = ((x / 6) + (y / 6)) % 2 == 0 ? 0.75f : 0.5f;
			const size_t i = (static_cast<size_t>(y) * w + x) * 4;
			px[i + 0] = uint8((rgb.r * a + bg * (1.f - a)) * 255.f);
			px[i + 1] = uint8((rgb.g * a + bg * (1.f - a)) * 255.f);
			px[i + 2] = uint8((rgb.b * a + bg * (1.f - a)) * 255.f);
			px[i + 3] = 255;
		}
	}
	return px;
}

std::string ColorPicker::FmtInt(int v) {
	return std::to_string(v);
}

std::string ColorPicker::FmtHex(vec4 c) {
	auto ch = [](float v) { return static_cast<int>(std::round(std::clamp(v, 0.f, 1.f) * 255.f)); };
	std::ostringstream ss;
	ss << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << ch(c.r) << std::setw(2) << ch(c.g)
	   << std::setw(2) << ch(c.b) << std::setw(2) << ch(c.a);
	return ss.str();
}

bool ColorPicker::ParseHex(const std::string &s, vec4 &out) {
	std::string h = s;
	if (!h.empty() && h[0] == '#') {
		h = h.substr(1);
	}
	if (h.size() == 6) {
		h += "FF";
	}
	if (h.size() != 8) {
		return false;
	}
	try {
		const uint32 v = static_cast<uint32>(std::stoul(h, nullptr, 16));
		out.r = ((v >> 24) & 0xFF) / 255.f;
		out.g = ((v >> 16) & 0xFF) / 255.f;
		out.b = ((v >> 8) & 0xFF) / 255.f;
		out.a = ((v >> 0) & 0xFF) / 255.f;
		return true;
	} catch (...) {
		return false;
	}
}

ColorPicker::ColorPicker(GFX::GfxContext &ctx, vec4 color) : m_Ctx(ctx), m_Color(color) {
	const vec3 hsv = RgbToHsv(color.r, color.g, color.b);
	m_H = hsv.x;
	m_S = hsv.y;
	m_V = hsv.z;

	StyleProperties sp;
	sp.flexDirection = FlexDirection::Column;
	sp.width = StyleLength::Grow();
	SetStyle(sp);

	Init();
}

void ColorPicker::SetColor(vec4 color) {
	m_Color = color;
	const vec3 hsv = RgbToHsv(color.r, color.g, color.b);
	m_H = hsv.x;
	m_S = hsv.y;
	m_V = hsv.z;
	RebuildSVTexture();
	RebuildAlphaTexture();
	SyncAll();
}

void ColorPicker::SetOnChanged(Delegate<void(vec4)> callback) {
	m_OnChanged = std::move(callback);
}

void ColorPicker::OnMousePress(Platform::MouseButton btn, vec2 pos) {
	View::OnMousePress(btn, pos);
}

void ColorPicker::OnDrawSelf(Rendering::DrawList &dl) {
	View::OnDrawSelf(dl);
}

void ColorPicker::RebuildSVTexture() {
	auto px = GenSV(m_H, kTexW, kSVTexH);
	if (!m_SVTex) {
		m_SVTex = MakeTex(m_Ctx, kTexW, kSVTexH, px, "ColorPicker_SV");
	} else {
		m_Ctx.UploadTextureData(*m_SVTex, px.data(), static_cast<uint64>(px.size()));
	}
	if (m_SVArea) {
		m_SVArea->m_Tex = m_SVTex.get();
		m_SVArea->QueueRedraw();
	}
}

void ColorPicker::RebuildAlphaTexture() {
	auto px = GenAlpha({ m_Color.r, m_Color.g, m_Color.b }, kTexW, kBarTexH);
	if (!m_AlphaTex) {
		m_AlphaTex = MakeTex(m_Ctx, kTexW, kBarTexH, px, "ColorPicker_Alpha");
	} else {
		m_Ctx.UploadTextureData(*m_AlphaTex, px.data(), static_cast<uint64>(px.size()));
	}
	if (m_AlphaBar) {
		m_AlphaBar->m_Tex = m_AlphaTex.get();
		m_AlphaBar->QueueRedraw();
	}
}

void ColorPicker::RebuildChannelTextures() {
	const bool isHSV = (m_Mode == Mode::HSV);
	static const char *names[4] = { "ColorPicker_Ch0", "ColorPicker_Ch1", "ColorPicker_Ch2", "ColorPicker_Ch3" };
	for (int i = 0; i < 4; ++i) {
		auto px = GenChannelGrad(m_Color, m_H, m_S, m_V, i, isHSV, kTexW, kBarTexH);
		if (!m_ChTex[i]) {
			m_ChTex[i] = MakeTex(m_Ctx, kTexW, kBarTexH, px, names[i]);
		} else {
			m_Ctx.UploadTextureData(*m_ChTex[i], px.data(), static_cast<uint64>(px.size()));
		}
		if (m_Ch[i].slider) {
			m_Ch[i].slider->SetTrackTexture(m_ChTex[i].get());
		}
	}
}

void ColorPicker::SyncChannelDisplays() {
	if (!m_Ch[0].slider) {
		return;
	}

	float vals[4] = {};
	if (m_Mode == Mode::RGB) {
		vals[0] = m_Color.r * 255.f;
		vals[1] = m_Color.g * 255.f;
		vals[2] = m_Color.b * 255.f;
		vals[3] = m_Color.a * 255.f;
		for (int i = 0; i < 4; ++i) {
			m_Ch[i].slider->SetValue(vals[i]);
			m_Ch[i].input->SetText(FmtInt(static_cast<int>(std::round(vals[i]))));
		}
	} else if (m_Mode == Mode::HSV) {
		vals[0] = m_H * 360.f;
		vals[1] = m_S * 100.f;
		vals[2] = m_V * 100.f;
		vals[3] = m_Color.a * 255.f;
		for (int i = 0; i < 4; ++i) {
			m_Ch[i].slider->SetValue(vals[i]);
			m_Ch[i].input->SetText(FmtInt(static_cast<int>(std::round(vals[i]))));
		}
	} else { // HEX
		m_Ch[3].slider->SetValue(m_Color.a * 255.f);
		m_Ch[3].input->SetText(FmtInt(static_cast<int>(std::round(m_Color.a * 255.f))));
	}
}

void ColorPicker::SyncHexDisplay() {
	if (m_HexInput) {
		m_HexInput->SetText(FmtHex(m_Color));
	}
}

void ColorPicker::SyncAll() {
	if (m_SVArea) {
		m_SVArea->m_Indicator = { m_S, 1.f - m_V };
		m_SVArea->QueueRedraw();
	}
	if (m_HueBar) {
		m_HueBar->m_Indicator = { m_H, 0.f };
		m_HueBar->QueueRedraw();
	}
	if (m_AlphaBar) {
		m_AlphaBar->m_Indicator = { m_Color.a, 0.f };
		m_AlphaBar->QueueRedraw();
	}
	if (m_Preview) {
		StyleProperties p;
		p.backgroundColor = m_Color;
		m_Preview->MergeStyle(p);
	}
	if (m_Swatch) {
		StyleProperties p;
		p.backgroundColor = m_Color;
		m_Swatch->MergeStyle(p);
	}
	RebuildChannelTextures();
	SyncChannelDisplays();
	SyncHexDisplay();
}

void ColorPicker::ApplyChannelValue(int idx, float rawValue) {
	if (m_Mode == Mode::RGB) {
		const float f = rawValue / 255.f;
		switch (idx) {
		case 0:
			m_Color.r = f;
			break;
		case 1:
			m_Color.g = f;
			break;
		case 2:
			m_Color.b = f;
			break;
		case 3:
			m_Color.a = f;
			break;
		}
		if (idx < 3) {
			const vec3 hsv = RgbToHsv(m_Color.r, m_Color.g, m_Color.b);
			m_H = hsv.x;
			m_S = hsv.y;
			m_V = hsv.z;
			RebuildSVTexture();
		}
		RebuildAlphaTexture();
	} else if (m_Mode == Mode::HSV) {
		switch (idx) {
		case 0:
			m_H = rawValue / 360.f;
			break;
		case 1:
			m_S = rawValue / 100.f;
			break;
		case 2:
			m_V = rawValue / 100.f;
			break;
		case 3:
			m_Color.a = rawValue / 255.f;
			break;
		}
		const vec3 rgb = HsvToRgb(m_H, m_S, m_V);
		m_Color.r = rgb.r;
		m_Color.g = rgb.g;
		m_Color.b = rgb.b;
		if (idx == 0) {
			RebuildSVTexture();
		}
		RebuildAlphaTexture();
	} else {
		m_Color.a = rawValue / 255.f;
	}

	SyncAll();
	if (m_OnChanged) {
		m_OnChanged(m_Color);
	}
}

void ColorPicker::TogglePopup() {
	if (!m_Popup) {
		return;
	}
	if (!m_Popup->IsOpen()) {
		const Rect swatchRect = m_Swatch->GetAbsoluteRect();
		FloatingConfig fc = m_Popup->GetFloating();
		if (swatchRect.position.x > m_Popup->GetParent()->GetLayoutRect().Width() * 0.5f) {
			fc.elementPoint = FloatingAttachPoint::RightTop;
			fc.parentPoint = FloatingAttachPoint::RightBottom;
		} else {
			fc.elementPoint = FloatingAttachPoint::LeftTop;
			fc.parentPoint = FloatingAttachPoint::LeftBottom;
		}
		m_Popup->SetFloating(fc);
	}
	m_Popup->Toggle();
}
void ColorPicker::SetMode(Mode mode) {
	m_Mode = mode;

	static const char *rgbLbls[4] = { "R", "G", "B", "A" };
	static const char *hsvLbls[4] = { "H", "S", "V", "A" };

	const bool isHex = (mode == Mode::HEX);
	const bool isHSV = (mode == Mode::HSV);

	{
		StyleProperties p;
		p.display = isHex ? Display::Flex : Display::None;
		m_HexRow->MergeStyle(p);
	}

	for (int i = 0; i < 3; ++i) {
		StyleProperties p;
		p.display = isHex ? Display::None : Display::Flex;
		m_Ch[i].row->MergeStyle(p);
	}

	if (!isHex) {
		const char **lbls = isHSV ? hsvLbls : rgbLbls;
		for (int i = 0; i < 4; ++i) {
			m_Ch[i].label->SetText(lbls[i]);
		}

		if (isHSV) {
			m_Ch[0].slider->SetRange(0.f, 360.f);
			m_Ch[1].slider->SetRange(0.f, 100.f);
			m_Ch[2].slider->SetRange(0.f, 100.f);
			m_Ch[3].slider->SetRange(0.f, 255.f);
		} else {
			for (int i = 0; i < 4; ++i) {
				m_Ch[i].slider->SetRange(0.f, 255.f);
			}
		}
	} else {
		m_Ch[3].label->SetText("A");
		m_Ch[3].slider->SetRange(0.f, 255.f);
	}

	RebuildChannelTextures();
	SyncChannelDisplays();
	SyncHexDisplay();
}

void ColorPicker::Init() {
	AddClass("color-picker");

	{
		auto btn = CreateUnique<Button>();
		btn->AddClass("cp-swatch");
		btn->SetOnClick([this] { TogglePopup(); });
		m_Swatch = AddChild(std::move(btn));
	}

	m_HueTex = MakeTex(m_Ctx, kTexW, kBarTexH, GenHue(kTexW, kBarTexH), "ColorPicker_Hue");
	RebuildSVTexture();
	RebuildAlphaTexture();

	{
		FloatingConfig fc;
		fc.attachTo = FloatingAttachTo::Parent;
		fc.elementPoint = FloatingAttachPoint::LeftTop;
		fc.parentPoint = FloatingAttachPoint::LeftBottom;
		fc.offset = { 0.f, 4.f };
		fc.zIndex = 60;

		auto popup = CreateUnique<Popup>();
		popup->AddClass("cp-popup");
		popup->SetFloating(fc);
		m_Popup = static_cast<Popup *>(AddChild(std::move(popup)));
	}

	{
		auto sv = CreateUnique<PickerArea>();
		sv->AddClass("cp-sv-area");
		sv->m_Tex = m_SVTex.get();
		sv->m_Is1D = false;
		sv->m_Indicator = { m_S, 1.f - m_V };
		sv->m_OnPick = [this](vec2 n) {
			m_S = n.x;
			m_V = 1.f - n.y;
			const vec3 rgb = HsvToRgb(m_H, m_S, m_V);
			m_Color.r = rgb.r;
			m_Color.g = rgb.g;
			m_Color.b = rgb.b;
			RebuildAlphaTexture();
			SyncAll();
			if (m_OnChanged) {
				m_OnChanged(m_Color);
			}
		};
		m_SVArea = static_cast<PickerArea *>(m_Popup->AddChild(std::move(sv)));
	}

	{
		auto row = CreateUnique<View>();
		row->AddClass("cp-hue-row");
		View *rowRaw = m_Popup->AddChild(std::move(row));

		{
			auto hue = CreateUnique<PickerArea>();
			hue->AddClass("cp-hue-bar");
			hue->m_Tex = m_HueTex.get();
			hue->m_Is1D = true;
			hue->m_Indicator = { m_H, 0.f };
			hue->m_OnPick = [this](vec2 n) {
				const bool changed = std::abs(n.x - m_H) > 1e-4f;
				m_H = n.x;
				const vec3 rgb = HsvToRgb(m_H, m_S, m_V);
				m_Color.r = rgb.r;
				m_Color.g = rgb.g;
				m_Color.b = rgb.b;
				if (changed) {
					RebuildSVTexture();
				}
				RebuildAlphaTexture();
				SyncAll();
				if (m_OnChanged) {
					m_OnChanged(m_Color);
				}
			};
			m_HueBar = static_cast<PickerArea *>(rowRaw->AddChild(std::move(hue)));
		}

		{
			auto prev = CreateUnique<View>();
			prev->AddClass("cp-preview");
			m_Preview = rowRaw->AddChild(std::move(prev));
		}
	}

	{
		auto alpha = CreateUnique<PickerArea>();
		alpha->AddClass("cp-alpha-bar");
		alpha->m_Tex = m_AlphaTex.get();
		alpha->m_Is1D = true;
		alpha->m_Indicator = { m_Color.a, 0.f };
		alpha->m_OnPick = [this](vec2 n) {
			m_Color.a = n.x;
			SyncAll();
			if (m_OnChanged) {
				m_OnChanged(m_Color);
			}
		};
		m_AlphaBar = static_cast<PickerArea *>(m_Popup->AddChild(std::move(alpha)));
	}

	{
		auto row = CreateUnique<View>();
		row->AddClass("cp-mode-row");
		View *rowRaw = m_Popup->AddChild(std::move(row));

		auto makeBtn = [&](const char *text) -> Button * {
			auto btn = CreateUnique<Button>(text);
			btn->AddClass("cp-mode-btn");
			return static_cast<Button *>(rowRaw->AddChild(std::move(btn)));
		};
		m_RGBBtn = makeBtn("RGB");
		m_HSVBtn = makeBtn("HSV");
		m_HEXBtn = makeBtn("HEX");
		m_RGBBtn->SetOnClick([this] { SetMode(Mode::RGB); });
		m_HSVBtn->SetOnClick([this] { SetMode(Mode::HSV); });
		m_HEXBtn->SetOnClick([this] { SetMode(Mode::HEX); });
	}

	const char *labels[4] = { "R", "G", "B", "A" };
	for (int i = 0; i < 4; ++i) {
		auto row = CreateUnique<View>();
		row->AddClass("cp-ch-row");
		m_Ch[i].row = m_Popup->AddChild(std::move(row));

		{
			auto lbl = CreateUnique<Label>(labels[i]);
			lbl->AddClass("cp-ch-lbl");
			m_Ch[i].label = static_cast<Label *>(m_Ch[i].row->AddChild(std::move(lbl)));
		}
		{
			auto sl = CreateUnique<Slider>();
			sl->SetRange(0.f, 255.f);
			sl->SetStep(1.f);
			sl->AddClass("cp-ch-slider");
			m_Ch[i].slider = static_cast<Slider *>(m_Ch[i].row->AddChild(std::move(sl)));
		}
		{
			auto ti = CreateUnique<TextInput>();
			ti->AddClass("cp-ch-input");
			m_Ch[i].input = static_cast<TextInput *>(m_Ch[i].row->AddChild(std::move(ti)));
		}

		const int idx = i;
		m_Ch[i].slider->SetOnChanged([this, idx](float val) {
			m_Ch[idx].input->SetText(FmtInt(static_cast<int>(std::round(val))));
			ApplyChannelValue(idx, val);
		});
		m_Ch[i].input->SetOnSubmit([this, idx](const std::string &s) {
			try {
				const float val = static_cast<float>(std::stoi(s));
				m_Ch[idx].slider->SetValue(val);
				ApplyChannelValue(idx, val);
			} catch (...) {
			}
		});
	}

	{
		auto row = CreateUnique<View>();
		row->AddClass("cp-hex-row");
		m_HexRow = m_Popup->AddChild(std::move(row));

		{
			auto lbl = CreateUnique<Label>("#");
			lbl->AddClass("cp-hex-lbl");
			m_HexRow->AddChild(std::move(lbl));
		}
		{
			auto ti = CreateUnique<TextInput>();
			ti->AddClass("cp-hex-input");
			ti->SetOnSubmit([this](const std::string &s) {
				vec4 c;
				if (ParseHex(s, c)) {
					m_Color = c;
					const vec3 hsv = RgbToHsv(c.r, c.g, c.b);
					m_H = hsv.x;
					m_S = hsv.y;
					m_V = hsv.z;
					RebuildSVTexture();
					RebuildAlphaTexture();
					SyncAll();
					if (m_OnChanged) {
						m_OnChanged(m_Color);
					}
				}
			});
			m_HexInput = static_cast<TextInput *>(m_HexRow->AddChild(std::move(ti)));
		}
	}

	RebuildChannelTextures();
	SyncAll();
}

} // namespace Aquila::UI::Core
