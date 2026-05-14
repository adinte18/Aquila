#pragma once

#include "Aquila/GFX/GfxContext.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Text/FontAtlas.h"
#include "Aquila/UI/Core/FontRegistry.h"
#include "Aquila/UI/Core/TextureCache.h"

namespace Aquila::Application {

void SetupWidgetTest(GFX::GfxContext &ctx, UI::Core::Canvas &canvas,
					 std::vector<Unique<UI::Text::FontAtlas>> &fonts,
					 std::vector<Unique<UI::Core::TextureCache>> &textureCaches);

} // namespace Aquila::Application
