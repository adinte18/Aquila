#pragma once

#include "Aquila/UI/Core/View.h"
#include <string>

namespace Aquila::GFX {
class GfxTexture;
}

namespace Aquila::UI::Core {

class DockPanel : public View {
  public:
	explicit DockPanel(std::string title = "");

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockPanel"; }
	[[nodiscard]] const std::string &GetTitle() const { return m_Title; }
	void SetTitle(std::string title);

	[[nodiscard]] GFX::GfxTexture *GetTabIcon() const { return m_TabIcon; }
	void SetTabIcon(GFX::GfxTexture *icon) { m_TabIcon = icon; }

	void ApplyXmlTextContent(std::string_view text) override { SetTitle(std::string(text)); }
	void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr) override;

  private:
	std::string m_Title;
	GFX::GfxTexture *m_TabIcon = nullptr;
};

} // namespace Aquila::UI::Core
