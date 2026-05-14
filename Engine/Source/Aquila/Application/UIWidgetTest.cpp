#include "UIWidgetTest.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Style/StyleParser.h"

namespace Aquila::Application {

void SetupWidgetTest(GFX::GfxContext &ctx, UI::Core::Canvas &canvas, std::vector<Unique<UI::Text::FontAtlas>> &fonts,
					 std::vector<Unique<UI::Core::TextureCache>> &textureCaches) {
	textureCaches.push_back(CreateUnique<UI::Core::TextureCache>(ctx, "C:/Programming/Aquila/Resources/"));
	UI::Core::TextureCache &cache = *textureCaches.back();

	auto addFont = [&](const char *name, const char *path) {
		fonts.push_back(UI::Text::FontAtlas::CreateFromFile(ctx, path, 48.f));
		UI::Core::FontRegistry::Register(name, fonts.back().get());
	};

	addFont("regular", "C:/Programming/Aquila/Resources/Engine/Fonts/Lexend/Lexend-Regular.ttf");
	addFont("thin", "C:/Programming/Aquila/Resources/Engine/Fonts/Lexend/Lexend-Thin.ttf");
	addFont("medium", "C:/Programming/Aquila/Resources/Engine/Fonts/Lexend/Lexend-Medium.ttf");
	addFont("bold", "C:/Programming/Aquila/Resources/Engine/Fonts/Lexend/Lexend-Bold.ttf");

	UI::StyleParser::LoadFile("C:/Programming/Aquila/Resources/Engine/UI/widget_test.aqstyle", canvas.GetStyleSheet());

	UI::Core::LayoutLoader loader;
	loader.RegisterFont("regular", fonts[0].get());
	loader.RegisterTextureCache(textureCaches.back().get());

	auto root = loader.LoadFile("C:/Programming/Aquila/Resources/Engine/UI/widget_test.aqlayout");
	if (!root) {
		AQUILA_LOG_ERROR("SetupWidgetTest: failed to load layout");
		return;
	}

	if (auto *view = root->FindById("btn-click")) {
		if (auto *btn = dynamic_cast<UI::Core::Button *>(view)) {
			btn->SetOnClick([]() { AQUILA_LOG_INFO("Button clicked!"); });
		}
	}

	canvas.GetRoot()->AddChild(std::move(root));
	canvas.ReloadStyles();
}

} // namespace Aquila::Application
