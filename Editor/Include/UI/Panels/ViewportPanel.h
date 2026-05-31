#pragma once

#include "UI/Panels/IEditorPanel.h"

namespace Aquila::GFX {
class GfxTexture;
}

namespace Aquila::UI::Core {
class Image;
}

namespace Editor {

class ViewportPanel : public IEditorPanel {
  public:
	explicit ViewportPanel(Aquila::GFX::GfxTexture &initialTexture);
	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
	void SetTexture(Aquila::GFX::GfxTexture *texture);

  private:
	Aquila::GFX::GfxTexture &m_InitialTexture;
	Aquila::UI::Core::Image *m_Image = nullptr;
};

} // namespace Editor
