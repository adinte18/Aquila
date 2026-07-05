#include "Aquila/UI/Widgets/AssetSlot.h"

namespace Aquila::UI::Core {

AssetSlot::AssetSlot() {
	m_IsAcceptingPayload = true;
	AddClass("asset-slot");

	auto label = CreateUnique<Label>("");
	label->AddClass("asset-slot-label");
	m_Label = static_cast<Label *>(AddChild(std::move(label)));

	auto clearBtn = CreateUnique<Button>("×");
	clearBtn->AddClass("asset-slot-clear");
	clearBtn->onClick.Connect([this] { Clear(); });
	m_ClearButton = static_cast<Button *>(AddChild(std::move(clearBtn)));

	UpdateDisplay();
}

AssetSlot::AssetSlot(std::string acceptedType) : AssetSlot() {
	m_AcceptedType = std::move(acceptedType);
	UpdateDisplay();
}

void AssetSlot::SetAcceptedType(std::string type) {
	m_AcceptedType = std::move(type);
	UpdateDisplay();
}

void AssetSlot::SetValue(AssetPayload asset) {
	m_Value = std::move(asset);
	m_HasValue = true;
	UpdateDisplay();
}

void AssetSlot::Clear() {
	m_HasValue = false;
	m_Value = {};
	UpdateDisplay();
	RemoveClass("asset-slot-filled");
	onChanged(std::nullopt);
}


void AssetSlot::OnDrop(DragState &state) {
	if (!state.payload.has_value()) {
		return;
	}

	try {
		AssetPayload payload = std::any_cast<AssetPayload>(state.payload);
		if (!IsCompatible(payload)) {
			RemoveClass("drag-over-invalid");
			return;
		}
		SetValue(payload);
		AddClass("asset-slot-filled");
		RemoveClass("drag-over");
		RemoveClass("drag-over-invalid");
		onChanged(m_Value);
	} catch (const std::bad_any_cast &) {
	}
}

void AssetSlot::OnDragEnter(DragState &state) {
	if (!state.payload.has_value()) {
		return;
	}
	try {
		AssetPayload payload = std::any_cast<AssetPayload>(state.payload);
		if (IsCompatible(payload)) {
			AddClass("drag-over");
		} else {
			AddClass("drag-over-invalid");
		}
	} catch (const std::bad_any_cast &) {
	}
}

void AssetSlot::OnDragLeave(DragState &) {
	RemoveClass("drag-over");
	RemoveClass("drag-over-invalid");
}

void AssetSlot::ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx) {
	if (name == "accept") {
		SetAcceptedType(std::string(value));
		return;
	}
	View::ApplyXmlAttribute(name, value, loaderCtx);
}

void AssetSlot::UpdateDisplay() {
	m_ClearButton->SetHidden(!m_HasValue);

	if (m_HasValue) {
		m_Label->SetText(m_Value.displayName.empty() ? m_Value.assetPath : m_Value.displayName);
	} else {
		std::string placeholder = "None";
		if (!m_AcceptedType.empty()) {
			placeholder += " (" + m_AcceptedType + ")";
		}
		m_Label->SetText(placeholder);
	}
}

bool AssetSlot::IsCompatible(const AssetPayload &payload) const {
	if (m_AcceptedType.empty()) {
		return true;
	}
	return payload.assetType == m_AcceptedType;
}

} // namespace Aquila::UI::Core
