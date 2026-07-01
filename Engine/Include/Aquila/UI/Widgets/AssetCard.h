#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/AssetPayload.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/UI/Core/DragState.h"

namespace Aquila::UI::Core {

class AssetCard : public View {
  public:
	AssetCard();

	[[nodiscard]] std::string_view GetTypeName() const override { return "AssetCard"; }

	void SetAsset(AssetPayload payload);
	[[nodiscard]] const AssetPayload &GetAsset() const { return m_Payload; }

	void SetThumbnail(GFX::GfxTexture *texture);

	void OnDragStart(DragState &state) override;

  private:
	AssetPayload m_Payload;

	Image *m_Thumbnail = nullptr;
	Label *m_TypeLabel = nullptr;
	Label *m_NameLabel = nullptr;
};

} // namespace Aquila::UI::Core
