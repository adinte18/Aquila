#include "Aquila/UI/Widgets/IconLabel.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Core {

IconLabel::IconLabel() {
	m_ShouldSkipHitTest = true;

	m_Icon = dynamic_cast<Image *>(AddChild(CreateUnique<Image>()));
	m_Icon->AddClass("icon");
	m_Label = dynamic_cast<Label *>(AddChild(CreateUnique<Label>(std::string())));

	UpdateIconVisibility();
}

IconLabel::IconLabel(std::string text, Text::FontAtlas *font) : IconLabel() {
	m_Label->SetText(std::move(text));
	m_Label->SetFont(font);
}

void IconLabel::SetText(std::string text) {
	m_Label->SetText(std::move(text));
}

void IconLabel::SetFont(Text::FontAtlas *font) {
	m_Label->SetFont(font);
}

void IconLabel::SetIconTexture(GFX::GfxTexture *texture) {
	m_Icon->SetTexture(texture);
	UpdateIconVisibility();
}

void IconLabel::SetIconTint(vec4 tint) {
	m_Icon->SetTint(tint);
}

void IconLabel::OnStyleResolved() {
	View::OnStyleResolved();
	if (Text::FontAtlas *font = GetResolvedFont()) {
		m_Label->SetFont(font);
	}
}

void IconLabel::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "src" || name == "icon" || name == "bank" || name == "uv" || name == "tint") {
		m_Icon->ApplyXmlAttribute(name, value, loaderCtx);
		UpdateIconVisibility();
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

void IconLabel::UpdateIconVisibility() {
	StyleProperties sp;
	sp.display = (m_Icon->GetTexture() != nullptr) ? Display::Flex : Display::None;
	m_Icon->MergeStyle(sp);
}

} // namespace Aquila::UI::Core
