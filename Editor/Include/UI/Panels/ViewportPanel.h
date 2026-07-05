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
	explicit ViewportPanel(Aquila::GFX::GfxTexture &initial_texture);
	void build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlay_root) override;
	void set_texture(Aquila::GFX::GfxTexture *texture);

  private:
	Aquila::GFX::GfxTexture &m_initial_texture;
	Aquila::UI::Core::Image *m_image = nullptr;
};

} // namespace Editor
