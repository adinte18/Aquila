#include "Aquila/UI/Widgets/AssetCard.h"

namespace Aquila::UI::Core {

AssetCard::AssetCard() {
	m_IsDraggable = true;
	AddClass("asset-card");

	auto thumbnail = CreateUnique<Image>();
	thumbnail->AddClass("asset-card-thumbnail");
	m_Thumbnail = static_cast<Image *>(AddChild(std::move(thumbnail)));

	auto typeLabel = CreateUnique<Label>("");
	typeLabel->AddClass("asset-card-type");
	m_TypeLabel = static_cast<Label *>(AddChild(std::move(typeLabel)));

	auto nameLabel = CreateUnique<Label>("");
	nameLabel->AddClass("asset-card-name");
	m_NameLabel = static_cast<Label *>(AddChild(std::move(nameLabel)));
}

void AssetCard::SetAsset(AssetPayload payload) {
	m_Payload = std::move(payload);

	std::string displayName = m_Payload.displayName.empty() ? m_Payload.assetPath : m_Payload.displayName;
	m_NameLabel->SetText(displayName);
	m_TypeLabel->SetText(m_Payload.assetType);
}

void AssetCard::SetThumbnail(GFX::GfxTexture *texture) {
	m_Thumbnail->SetTexture(texture);

	m_Thumbnail->SetHidden(texture == nullptr);
}

void AssetCard::OnDragStart(DragState &state) {
	state.payload = m_Payload;
}

} // namespace Aquila::UI::Core
