#ifndef EDITOR_ABOUT_WINDOW_H
#define EDITOR_ABOUT_WINDOW_H

#include "Aquila/Core/Layer.h"

namespace Editor::UI {

/**
 * @brief About window displaying editor information
 * 
 * Shows version info, credits, system information, and links
 */
class AboutWindow : public Aquila::Core::Layer {
public:
	AboutWindow();
	~AboutWindow() override = default;

	void OnImGuiRender() override;

	void Show() { m_IsOpen = true; }
	void Hide() { m_IsOpen = false; }
	bool IsOpen() const { return m_IsOpen; }
	void SetOpen(bool open) { m_IsOpen = open; }

private:
	void RenderSystemInfo();
	void RenderCredits();
	void RenderLicenses();

	bool m_IsOpen = false;
	int m_CurrentTab = 0;
};

} // namespace Editor::UI

#endif // EDITOR_ABOUT_WINDOW_H
