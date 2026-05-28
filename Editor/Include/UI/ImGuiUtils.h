#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include <string>
#include <functional>

namespace ImGuiUtils {

enum class AlignX { Left, Center, Right, Keep };

enum class AlignY { Top, Center, Bottom, Keep };

inline void TextCentered(const char *text) {
	f32 windowWidth = ImGui::GetWindowSize().x;
	f32 textWidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::Text("%s", text);
}

inline void TextCenteredFormatted(const char *fmt, ...) {
	char buffer[1024];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextCentered(buffer);
}

template <typename Func>
inline void AlignedWidget(AlignX alignX, AlignY alignY, const ImVec2 &padding, const ImVec2 &size, Func &&drawFunc) {
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	if (window->SkipItems) {
		return;
	}

	ImVec2 cursor = ImGui::GetCursorPos();

	ImVec2 finalPos = cursor;

	ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
	ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
	ImVec2 contentSize = { contentMax.x - contentMin.x, contentMax.y - contentMin.y };

	if (alignX == AlignX::Left) {
		finalPos.x = contentMin.x + padding.x;
	} else if (alignX == AlignX::Center) {
		finalPos.x = contentMin.x + (contentSize.x - size.x) * 0.5f;
	} else if (alignX == AlignX::Right) {
		finalPos.x = contentMax.x - size.x - padding.x;
	}

	if (alignY == AlignY::Top) {
		finalPos.y = contentMin.y + padding.y;
	} else if (alignY == AlignY::Center) {
		finalPos.y = cursor.y + (contentSize.y - size.y) * 0.5f;
	} else if (alignY == AlignY::Bottom) {
		finalPos.y = contentMax.y - size.y - padding.y;
	}

	ImGui::SetCursorPos(finalPos);
	drawFunc();
}

namespace Internal {
constexpr size_t MAX_TEXT_BUFFER = 4096;

inline int SafeVFormat(char *buffer, size_t bufferSize, const char *fmt, va_list args) {
	if (!buffer || bufferSize == 0 || !fmt) {
		return -1;
	}

	int result = vsnprintf(buffer, bufferSize, fmt, args);

	buffer[bufferSize - 1] = '\0';

	if (result < 0 || static_cast<size_t>(result) >= bufferSize) {
#ifdef _DEBUG
		ImGui::DebugLog("Warning: Text truncated in ImGuiUtils (needed %d bytes, had %zu)\n", result, bufferSize);
#endif
		return -1;
	}

	return result;
}

inline bool SafeFormat(char *buffer, size_t bufferSize, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	int result = SafeVFormat(buffer, bufferSize, fmt, args);
	va_end(args);
	return result >= 0;
}
} // namespace Internal

inline void TextTopLeft(const char *text, const ImVec2 &padding = ImVec2(0, 0)) {
	ImGui::SetCursorPosX(padding.x);
	ImGui::SetCursorPosY(padding.y);
	ImGui::Text("%s", text);
}

inline void TextTopLeftFmt(const ImVec2 &padding, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextTopLeft(buffer, padding);
}

inline void TextTopCenter(const char *text, f32 paddingY = 0.0f) {
	f32 windowWidth = ImGui::GetWindowSize().x;
	f32 textWidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
	ImGui::SetCursorPosY(paddingY);
	ImGui::Text("%s", text);
}

inline void TextTopCenterFmt(f32 paddingY, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextTopCenter(buffer, paddingY);
}

inline void TextTopRight(const char *text, const ImVec2 &padding = ImVec2(0, 0)) {
	f32 windowWidth = ImGui::GetWindowSize().x;
	f32 textWidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX(windowWidth - textWidth - padding.x);
	ImGui::SetCursorPosY(padding.y);
	ImGui::Text("%s", text);
}

inline void TextTopRightFmt(const ImVec2 &padding, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextTopRight(buffer, padding);
}

inline void TextMiddleLeft(const char *text, f32 paddingX = 0.0f) {
	f32 windowHeight = ImGui::GetWindowSize().y;
	f32 textHeight = ImGui::CalcTextSize(text).y;
	ImGui::SetCursorPosX(paddingX);
	ImGui::SetCursorPosY((windowHeight - textHeight) * 0.5f);
	ImGui::Text("%s", text);
}

inline void TextMiddleLeftFmt(f32 paddingX, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextMiddleLeft(buffer, paddingX);
}

inline void TextMiddleCenter(const char *text) {
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 textSize = ImGui::CalcTextSize(text);
	ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
	ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
	ImGui::Text("%s", text);
}

inline void TextMiddleCenterFmt(const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextMiddleCenter(buffer);
}

inline void TextMiddleRight(const char *text, f32 paddingX = 0.0f) {
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 textSize = ImGui::CalcTextSize(text);
	ImGui::SetCursorPosX(windowSize.x - textSize.x - paddingX);
	ImGui::SetCursorPosY((windowSize.y - textSize.y) * 0.5f);
	ImGui::Text("%s", text);
}

inline void TextMiddleRightFmt(f32 paddingX, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextMiddleRight(buffer, paddingX);
}

inline void TextBottomLeft(const char *text, const ImVec2 &padding = ImVec2(0, 0)) {
	f32 windowHeight = ImGui::GetWindowSize().y;
	f32 textHeight = ImGui::CalcTextSize(text).y;
	ImGui::SetCursorPosX(padding.x);
	ImGui::SetCursorPosY(windowHeight - textHeight - padding.y);
	ImGui::Text("%s", text);
}

inline void TextBottomLeftFmt(const ImVec2 &padding, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextBottomLeft(buffer, padding);
}

inline void TextBottomCenter(const char *text, f32 paddingY = 0.0f) {
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 textSize = ImGui::CalcTextSize(text);
	ImGui::SetCursorPosX((windowSize.x - textSize.x) * 0.5f);
	ImGui::SetCursorPosY(windowSize.y - textSize.y - paddingY);
	ImGui::Text("%s", text);
}

inline void TextBottomCenterFmt(f32 paddingY, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextBottomCenter(buffer, paddingY);
}

inline void TextBottomRight(const char *text, const ImVec2 &padding = ImVec2(0, 0)) {
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 textSize = ImGui::CalcTextSize(text);
	ImGui::SetCursorPosX(windowSize.x - textSize.x - padding.x);
	ImGui::SetCursorPosY(windowSize.y - textSize.y - padding.y);
	ImGui::Text("%s", text);
}

inline void TextBottomRightFmt(const ImVec2 &padding, const char *fmt, ...) {
	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	TextBottomRight(buffer, padding);
}

enum class TextAlign {
	TopLeft,
	TopCenter,
	TopRight,
	MiddleLeft,
	MiddleCenter,
	MiddleRight,
	BottomLeft,
	BottomCenter,
	BottomRight
};

inline void TextAligned(const char *text, TextAlign align, const ImVec2 &padding = ImVec2(0, 0)) {
	if (!text) {
		return;
	}

	switch (align) {
	case TextAlign::TopLeft:
		TextTopLeft(text, padding);
		break;
	case TextAlign::TopCenter:
		TextTopCenter(text, padding.y);
		break;
	case TextAlign::TopRight:
		TextTopRight(text, padding);
		break;
	case TextAlign::MiddleLeft:
		TextMiddleLeft(text, padding.x);
		break;
	case TextAlign::MiddleCenter:
		TextMiddleCenter(text);
		break;
	case TextAlign::MiddleRight:
		TextMiddleRight(text, padding.x);
		break;
	case TextAlign::BottomLeft:
		TextBottomLeft(text, padding);
		break;
	case TextAlign::BottomCenter:
		TextBottomCenter(text, padding.y);
		break;
	case TextAlign::BottomRight:
		TextBottomRight(text, padding);
		break;
	}
}

inline void TextAlignedFmt(TextAlign align, const ImVec2 &padding, const char *fmt, ...) {
	if (!fmt) {
		return;
	}

	char buffer[Internal::MAX_TEXT_BUFFER];
	va_list args;
	va_start(args, fmt);
	Internal::SafeVFormat(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	TextAligned(buffer, align, padding);
}

inline void CenterNextItem(f32 itemWidth) {
	f32 windowWidth = ImGui::GetWindowSize().x;
	ImGui::SetCursorPosX((windowWidth - itemWidth) * 0.5f);
}

template <typename Func> inline void HorizontalGroup(TextAlign align, const ImVec2 &padding, Func &&renderFunc) {
	ImVec2 startPos = ImGui::GetCursorPos();

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.0f);
	ImGui::BeginGroup();
	renderFunc();
	ImGui::EndGroup();
	ImGui::PopStyleVar();

	ImVec2 groupMin = ImGui::GetItemRectMin();
	ImVec2 groupMax = ImGui::GetItemRectMax();
	ImVec2 groupSize = ImVec2(groupMax.x - groupMin.x, groupMax.y - groupMin.y);

	ImGui::SetCursorPos(startPos);

	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 finalPos;

	switch (align) {
	case TextAlign::TopLeft:
		finalPos = ImVec2(padding.x, padding.y);
		break;
	case TextAlign::TopCenter:
		finalPos = ImVec2((windowSize.x - groupSize.x) * 0.5f, padding.y);
		break;
	case TextAlign::TopRight:
		finalPos = ImVec2(windowSize.x - groupSize.x - padding.x, padding.y);
		break;
	case TextAlign::MiddleLeft:
		finalPos = ImVec2(padding.x, (windowSize.y - groupSize.y) * 0.5f);
		break;
	case TextAlign::MiddleCenter:
		finalPos = ImVec2((windowSize.x - groupSize.x) * 0.5f, (windowSize.y - groupSize.y) * 0.5f);
		break;
	case TextAlign::MiddleRight:
		finalPos = ImVec2(windowSize.x - groupSize.x - padding.x, (windowSize.y - groupSize.y) * 0.5f);
		break;
	case TextAlign::BottomLeft:
		finalPos = ImVec2(padding.x, windowSize.y - groupSize.y - padding.y);
		break;
	case TextAlign::BottomCenter:
		finalPos = ImVec2((windowSize.x - groupSize.x) * 0.5f, windowSize.y - groupSize.y - padding.y);
		break;
	case TextAlign::BottomRight:
		finalPos = ImVec2(windowSize.x - groupSize.x - padding.x, windowSize.y - groupSize.y - padding.y);
		break;
	}

	ImGui::SetCursorPos(finalPos);

	ImGui::BeginGroup();
	renderFunc();
	ImGui::EndGroup();
}

template <typename Func> inline void CenteredHorizontal(Func &&renderFunc) {
	HorizontalGroup(TextAlign::TopCenter, ImVec2(0, 0), std::forward<Func>(renderFunc));
}

inline bool ButtonCentered(const char *label, const ImVec2 &size = ImVec2(0, 0)) {
	ImVec2 buttonSize = size;
	if (buttonSize.x == 0) {
		buttonSize.x = ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2;
	}
	CenterNextItem(buttonSize.x);
	return ImGui::Button(label, buttonSize);
}

inline bool SearchBar(const char *id, char *buffer, size_t bufferSize, const char *hint, f32 width = 200.0f) {
	ImGui::SetNextItemWidth(width);
	return ImGui::InputTextWithHint(id, hint, buffer, bufferSize);
}

inline void TextRight(const char *text, f32 offset = 0.0f) {
	f32 windowWidth = ImGui::GetWindowSize().x;
	f32 textWidth = ImGui::CalcTextSize(text).x;
	ImGui::SetCursorPosX(windowWidth - textWidth - offset);
	ImGui::Text("%s", text);
}

inline void CenterCursorY(f32 itemHeight) {
	f32 windowHeight = ImGui::GetWindowSize().y;
	ImGui::SetCursorPosY((windowHeight - itemHeight) * 0.5f);
}

inline void CenterCursor(const ImVec2 &itemSize) {
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::SetCursorPosX((windowSize.x - itemSize.x) * 0.5f);
	ImGui::SetCursorPosY((windowSize.y - itemSize.y) * 0.5f);
}

inline void SameLineRight(f32 width, f32 offset = 0.0f) {
	f32 windowWidth = ImGui::GetWindowSize().x;
	ImGui::SameLine(windowWidth - width - offset);
}

inline void BeginVerticalCenter(f32 contentHeight) {
	f32 windowHeight = ImGui::GetWindowSize().y;
	f32 offset = (windowHeight - contentHeight) * 0.5f;
	if (offset > 0) {
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offset);
	}
}

inline void SeparatorText(const char *text) {
	ImGui::Spacing();
	f32 windowWidth = ImGui::GetContentRegionAvail().x;
	f32 textWidth = ImGui::CalcTextSize(text).x;
	f32 separatorWidth = (windowWidth - textWidth) * 0.5f - 10.0f;

	ImGui::Separator();
	ImGui::SameLine(0, separatorWidth);
	ImGui::Text("%s", text);
	ImGui::SameLine(0, separatorWidth);
	ImGui::Separator();
}

inline void TextColored(const char *text, const ImVec4 &color) {
	ImGui::PushStyleColor(ImGuiCol_Text, color);
	ImGui::Text("%s", text);
	ImGui::PopStyleColor();
}

namespace Colors {
inline ImVec4 Red() {
	return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
}
inline ImVec4 Green() {
	return ImVec4(0.3f, 1.0f, 0.3f, 1.0f);
}
inline ImVec4 Blue() {
	return ImVec4(0.3f, 0.3f, 1.0f, 1.0f);
}
inline ImVec4 Yellow() {
	return ImVec4(1.0f, 1.0f, 0.3f, 1.0f);
}
inline ImVec4 Orange() {
	return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);
}
inline ImVec4 Gray() {
	return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
}
} // namespace Colors

inline void HelpMarker(const char *desc) {
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

struct ScopedIndent {
	ScopedIndent(f32 indent = 0.0f) { ImGui::Indent(indent == 0.0f ? ImGui::GetTreeNodeToLabelSpacing() : indent); }
	~ScopedIndent() { ImGui::Unindent(); }
};

struct ScopedDisable {
	ScopedDisable(bool disabled = true) : m_disabled(disabled) {
		if (m_disabled) {
			ImGui::BeginDisabled();
		}
	}
	~ScopedDisable() {
		if (m_disabled) {
			ImGui::EndDisabled();
		}
	}

  private:
	bool m_disabled;
};

struct ScopedColor {
	ScopedColor(ImGuiCol idx, const ImVec4 &color) { ImGui::PushStyleColor(idx, color); }
	~ScopedColor() { ImGui::PopStyleColor(); }
};

inline bool BeginCenteredWindow(const char *name, bool *p_open = nullptr, ImGuiWindowFlags flags = 0) {
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	return ImGui::Begin(name, p_open, flags);
}

inline void LoadingSpinner(const char *label, f32 radius = 15.0f, f32 thickness = 3.0f,
						   const ImVec4 &color = ImVec4(1, 1, 1, 1)) {
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	if (window->SkipItems) {
		return;
	}

	ImGuiContext &g = *GImGui;
	const ImGuiStyle &style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id)) {
		return;
	}

	window->DrawList->PathClear();

	int num_segments = 30;
	int start = abs(ImSin(g.Time * 1.8f) * (num_segments - 5));

	const f32 a_min = IM_PI * 2.0f * ((f32)start) / (f32)num_segments;
	const f32 a_max = IM_PI * 2.0f * ((f32)num_segments - 3) / (f32)num_segments;

	const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

	for (int i = 0; i < num_segments; i++) {
		const f32 a = a_min + ((f32)i / (f32)num_segments) * (a_max - a_min);
		window->DrawList->PathLineTo(
			ImVec2(centre.x + ImCos(a + g.Time * 8) * radius, centre.y + ImSin(a + g.Time * 8) * radius));
	}

	window->DrawList->PathStroke(ImGui::GetColorU32(color), false, thickness);
}

namespace DragDrop {

inline bool IsDragging(const char *payloadType) {
	const ImGuiPayload *payload = ImGui::GetDragDropPayload();
	return payload != nullptr && payload->IsDataType(payloadType);
}

template <typename T>
inline const T *AcceptTarget(const char *payloadType, const ImVec4 &overlayColor = ImVec4(0.3f, 0.5f, 0.8f, 0.3f)) {
	if (ImGui::BeginDragDropTarget()) {
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::GetColorU32(overlayColor));

		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(payloadType)) {
			ImGui::EndDragDropTarget();
			return static_cast<const T *>(payload->Data);
		}
		ImGui::EndDragDropTarget();
	}
	return nullptr;
}

inline const char *AcceptTargetString(const char *payloadType,
									  const ImVec4 &overlayColor = ImVec4(0.3f, 0.5f, 0.8f, 0.3f)) {
	if (ImGui::BeginDragDropTarget()) {
		ImVec2 min = ImGui::GetItemRectMin();
		ImVec2 max = ImGui::GetItemRectMax();
		ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::GetColorU32(overlayColor));

		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(payloadType)) {
			ImGui::EndDragDropTarget();
			return static_cast<const char *>(payload->Data);
		}
		ImGui::EndDragDropTarget();
	}
	return nullptr;
}

inline const char *DropZone(const char *label, const char *payloadType, const ImVec2 &size = ImVec2(0, 100),
							const char *hintText = "Drop here") {
	ImGui::BeginChild(label, size, true);

	bool isDragging = IsDragging(payloadType);

	if (isDragging) {
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.5f, 0.8f, 1.0f));
	}

	ImVec2 textSize = ImGui::CalcTextSize(hintText);
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::SetCursorPos(ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f));

	if (isDragging) {
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.8f, 1.0f), "%s", hintText);
	} else {
		ImGui::TextDisabled("%s", hintText);
	}

	const char *result = nullptr;

	if (ImGui::BeginDragDropTarget()) {
		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(payloadType)) {
			result = static_cast<const char *>(payload->Data);
		}
		ImGui::EndDragDropTarget();
	}

	if (isDragging) {
		ImGui::PopStyleColor();
	}

	ImGui::EndChild();
	return result;
}

inline void SetSource(const char *payloadType, const void *data, size_t dataSize, const char *preview = nullptr) {
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
		ImGui::SetDragDropPayload(payloadType, data, dataSize);
		if (preview) {
			ImGui::Text("%s", preview);
		}
		ImGui::EndDragDropSource();
	}
}

inline void SetSourceString(const char *payloadType, const char *str, const char *preview = nullptr) {
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
		ImGui::SetDragDropPayload(payloadType, str, strlen(str) + 1);
		if (preview) {
			ImGui::Text("%s", preview);
		} else {
			ImGui::Text("%s", str);
		}
		ImGui::EndDragDropSource();
	}
}

inline void AssetDropZone(const char *label, const char *assetType, const std::string &currentAsset,
						  const char *dragDropId, const std::function<void(const char *)> &onDrop,
						  const std::function<void()> &onClear = nullptr) {
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
	const ImVec2 availSize = ImVec2(ImGui::GetContentRegionAvail().x, 22.0f);

	constexpr ImVec4 bgColor = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
	constexpr ImVec4 borderColor = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);

	drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + availSize.x, cursorPos.y + availSize.y),
							ImGui::GetColorU32(bgColor), 3.0f);

	drawList->AddLine(ImVec2(cursorPos.x + 1, cursorPos.y + 1), ImVec2(cursorPos.x + availSize.x - 1, cursorPos.y + 1),
					  IM_COL32(0, 0, 0, 60), 1.0f);
	drawList->AddLine(ImVec2(cursorPos.x + 1, cursorPos.y + 1), ImVec2(cursorPos.x + 1, cursorPos.y + availSize.y - 1),
					  IM_COL32(0, 0, 0, 60), 1.0f);

	drawList->AddRect(cursorPos, ImVec2(cursorPos.x + availSize.x, cursorPos.y + availSize.y),
					  ImGui::GetColorU32(borderColor), 3.0f, 0, 1.0f);

	ImGui::InvisibleButton("##dropzone", availSize);
	const bool isHovered = ImGui::IsItemHovered();

	bool isDragging = false;
	if (ImGui::BeginDragDropTarget()) {
		isDragging = true;

		drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + availSize.x, cursorPos.y + availSize.y),
								IM_COL32(50, 120, 200, 50), 3.0f);
		drawList->AddRect(cursorPos, ImVec2(cursorPos.x + availSize.x, cursorPos.y + availSize.y),
						  IM_COL32(80, 150, 255, 220), 3.0f, 0, 2.0f);

		if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(dragDropId)) {
			const char *path = static_cast<const char *>(payload->Data);
			if (path && onDrop) {
				onDrop(path);
			}
		}

		ImGui::EndDragDropTarget();
	}

	if (isHovered && !isDragging) {
		drawList->AddRect(cursorPos, ImVec2(cursorPos.x + availSize.x, cursorPos.y + availSize.y),
						  IM_COL32(60, 60, 60, 180), 3.0f, 0, 1.5f);
	}

	const f32 centerY = cursorPos.y + (availSize.y * 0.5f) - 7.0f;
	const ImVec2 contentPos = ImVec2(cursorPos.x + 8, centerY);

	if (currentAsset.empty() || currentAsset == "None") {
		ImGui::SetCursorScreenPos(contentPos);
		ImGui::TextColored(ImVec4(0.45f, 0.45f, 0.45f, 1.0f), "None");

		ImGui::SameLine(0, 6);
		ImGui::TextColored(ImVec4(0.35f, 0.35f, 0.35f, 1.0f), "(%s)", assetType);
	} else {
		std::string displayName = currentAsset;

		if (const size_t lastSlash = currentAsset.find_last_of("/\\"); lastSlash != std::string::npos) {
			displayName = currentAsset.substr(lastSlash + 1);
		}

		if (const size_t lastDot = displayName.find_last_of('.'); lastDot != std::string::npos) {
			displayName = displayName.substr(0, lastDot);
		}

		ImGui::SetCursorScreenPos(contentPos);

#ifdef ICON_LC_FILE
		ImGui::Text(ICON_LC_FILE);
		ImGui::SameLine(0, 4);
#endif

		ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.85f, 1.0f), "%s", displayName.c_str());

		ImGui::SameLine(0, 6);
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(%s)", assetType);

		if (onClear) {
			const ImVec2 clearBtnPos = ImVec2(cursorPos.x + availSize.x - 24, cursorPos.y + 1);
			ImGui::SetCursorScreenPos(clearBtnPos);

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.25f, 0.25f, 0.9f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

#ifdef ICON_LC_X
			if (ImGui::Button(ICON_LC_X "##clear", ImVec2(15, 15))) {
#else
			if (ImGui::Button("X##clear", ImVec2(15, 15))) {
#endif
				onClear();
			}

			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("Clear");
			}

			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(3);
		}
	}

	ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, cursorPos.y + availSize.y + 2));
	ImGui::Dummy(ImVec2(0, 0));
}
} // namespace DragDrop

} // namespace ImGuiUtils
