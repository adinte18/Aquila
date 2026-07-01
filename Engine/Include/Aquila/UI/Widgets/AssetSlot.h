#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/AssetPayload.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Core/DragState.h"

namespace Aquila::UI::Core {

class AssetSlot : public View {
  public:
	AssetSlot();
	explicit AssetSlot(std::string acceptedType);

	[[nodiscard]] std::string_view GetTypeName() const override { return "AssetSlot"; }

	void SetAcceptedType(std::string type);
	[[nodiscard]] const std::string &GetAcceptedType() const { return m_AcceptedType; }

	void SetValue(AssetPayload asset);
	void Clear();
	[[nodiscard]] bool HasValue() const { return m_HasValue; }
	[[nodiscard]] const AssetPayload &GetValue() const { return m_Value; }

	Signal<void(Option<AssetPayload>)> onChanged;

	void OnDrop(DragState &state) override;
	void OnDragEnter(DragState &state) override;
	void OnDragLeave(DragState &state) override;
	void ApplyXmlAttribute(std::string_view name, std::string_view value, void *loaderCtx = nullptr) override;

  private:
	void UpdateDisplay();
	[[nodiscard]] bool IsCompatible(const AssetPayload &payload) const;

	std::string m_AcceptedType;
	AssetPayload m_Value;
	bool m_HasValue = false;

	Label *m_Label = nullptr;
	Button *m_ClearButton = nullptr;

};

} // namespace Aquila::UI::Core
