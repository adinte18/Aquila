#pragma once

#include "Aquila/Application/ApplicationNew.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/UI/Core/TextureCache.h"

namespace Aquila::UI::Core {
class Image;
}

namespace Editor {

class EditorApplication : public Aquila::Application::Application {
  public:
	explicit EditorApplication(const ApplicationSpec &spec);
	~EditorApplication() override = default;

  protected:
	void OnInit() override;
	void OnShutdown() override;
	void OnPreRender(f32 deltaTime) override;
	void OnEvent(Aquila::Application::Events::Event &event) override;
	void OnResize(uint32 width, uint32 height) override;

  private:
	void SetupScene();
	void SetupEditorUI();

	std::vector<Unique<Aquila::UI::Text::FontAtlas>> m_Fonts;
	std::vector<Unique<Aquila::UI::Core::TextureCache>> m_TextureCaches;
	Aquila::UI::Core::Image *m_ViewportImage = nullptr;
};

} // namespace Editor
