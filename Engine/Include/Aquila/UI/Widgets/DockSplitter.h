#pragma once

#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Widgets/DockTypes.h"

namespace Aquila::UI::Core {

class DockSplitter : public View {
  public:
	explicit DockSplitter(SplitDirection dir);

	[[nodiscard]] std::string_view GetTypeName() const override { return "DockSplitter"; }

	void SetSiblings(View *before, View *after);
	void UpdateSiblingRef(View *old, View *newPtr);
	void SetResizeBefore(bool v) { m_ResizeBefore = v; }

	[[nodiscard]] View *GetBefore() const { return m_Before; }
	[[nodiscard]] View *GetAfter() const { return m_After; }

	void OnMousePress(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override;
	void OnMouseMove(vec2 pos) override;

  private:
	SplitDirection m_Dir;
	View *m_Before = nullptr;
	View *m_After = nullptr;
	bool m_ResizeBefore = true;
	vec2 m_DragStartPos{};
	float m_BeforeGrowStart = 0.f;
	float m_AfterGrowStart = 0.f;
};

} // namespace Aquila::UI::Core
