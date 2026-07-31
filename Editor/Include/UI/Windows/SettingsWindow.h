#pragma once

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Core/EditorConfig.h"

#include <string>
#include <vector>

namespace Aquila::Graphics {
class QuadBatcher;
}
namespace Aquila::GFX {
class GfxCommandList;
class GfxTexture;
}
namespace Aquila::Application::Events {
class Event;
}
namespace Aquila::UI::Core {
class Canvas;
class View;
class TextureCache;
} // namespace Aquila::UI::Core

namespace Editor {

class SettingsWindow {
  public:
	SettingsWindow();
	~SettingsWindow();

	void build(Aquila::UI::Core::TextureCache *texture_cache, Uint32 width, Uint32 height,
			   const std::string &style_path);

	void update(F32 delta_time);
	void render(Aquila::Graphics::QuadBatcher &batcher, Aquila::GFX::GfxCommandList &cmd);
	void on_event(Aquila::Application::Events::Event &event);

	Delegate<void()> on_request_close;
	Delegate<void()> on_applied;

  private:
	Aquila::UI::Core::View *add_section(Aquila::UI::Core::View *host, const std::string &title);
	Aquila::UI::Core::View *make_row(Aquila::UI::Core::View *host, const std::string &label,
									 const std::string &description);
	void float_row(Aquila::UI::Core::View *host, const std::string &label, const std::string &description, float *field,
				   float min, float max, float speed, int precision);
	void font_row(Aquila::UI::Core::View *host, const std::string &label, const std::string &description,
				  std::string *family_field);

	void apply();
	void reset();
	void sync_widgets();

	Unique<Aquila::UI::Core::Canvas> m_canvas;
	Aquila::UI::Core::TextureCache *m_texture_cache = nullptr;
	Aquila::GFX::GfxTexture *m_help_icon = nullptr;
	Config::EditorPreferences m_working;
	std::vector<Delegate<void()>> m_sync;
};

} // namespace Editor
