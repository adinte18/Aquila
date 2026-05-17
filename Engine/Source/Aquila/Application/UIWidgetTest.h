#pragma once

#include "Aquila/GFX/GfxContext.h"
#include "Aquila/GFX/GfxTexture.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/UI/Widgets/Image.h"

namespace Aquila::Application {

// Returns the Image widget bound to the viewport slot, or nullptr if the layout didn't load.
UI::Core::Image *SetupWidgetTest(GFX::GfxContext &ctx, UI::Core::Canvas &canvas,
								 std::vector<Unique<UI::Text::FontAtlas>> &fonts,
								 std::vector<Unique<UI::Core::TextureCache>> &textureCaches,
								 GFX::GfxTexture *viewportTexture);

} // namespace Aquila::Application
