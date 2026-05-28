#include "UIWidgetTest.h"

#include "Aquila/Foundation/Macros.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/LayoutLoader.h"
#include "Aquila/UI/Style/StyleParser.h"

namespace Aquila::Application {

UI::Core::Image *SetupWidgetTest(GFX::GfxContext &ctx, UI::Core::Canvas &canvas,
								 std::vector<Unique<UI::Text::FontAtlas>> &fonts,
								 std::vector<Unique<UI::Core::TextureCache>> &textureCaches,
								 GFX::GfxTexture *viewportTexture) {
	textureCaches.push_back(CreateUnique<UI::Core::TextureCache>(ctx, "/resources"));
	UI::Core::TextureCache &cache = *textureCaches.back();

	auto addFont = [&](const char *name, const char *path) {
		fonts.push_back(UI::Text::FontAtlas::CreateFromFile(ctx, path, 48.f));
		UI::Core::FontRegistry::Register(name, fonts.back().get());
	};

	addFont("regular", "/resources/Engine/Fonts/Lexend/Lexend-Regular.ttf");
	addFont("thin", "/resources/Engine/Fonts/Lexend/Lexend-Thin.ttf");
	addFont("medium", "/resources/Engine/Fonts/Lexend/Lexend-Medium.ttf");
	addFont("bold", "/resources/Engine/Fonts/Lexend/Lexend-Bold.ttf");

	UI::StyleParser::LoadFile("/resources/Engine/UI/widget_test.aqstyle", canvas.GetStyleSheet());

	UI::Core::LayoutLoader loader;
	loader.RegisterFont("regular", fonts[0].get());
	loader.RegisterTextureCache(textureCaches.back().get());

	auto root = loader.LoadFile("/resources/Engine/UI/widget_test.aqlayout");
	if (!root) {
		AQUILA_LOG_ERROR("SetupWidgetTest: failed to load layout");
		return nullptr;
	}

	if (auto *view = root->FindById("btn-click")) {
		if (auto *btn = dynamic_cast<UI::Core::Button *>(view)) {
			btn->SetOnClick([]() { AQUILA_LOG_INFO("Button clicked!"); });
		}
	}

	UI::Core::Image *viewportImage = nullptr;
	if (auto *view = root->FindById("viewport")) {
		if (auto *img = dynamic_cast<UI::Core::Image *>(view)) {
			img->SetTexture(viewportTexture);
			viewportImage = img;
		}
	}

	canvas.GetRoot()->AddChild(std::move(root));
	canvas.ReloadStyles();
	return viewportImage;
}

} // namespace Aquila::Application
