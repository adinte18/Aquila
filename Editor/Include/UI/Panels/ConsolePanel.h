#pragma once

#include "UI/Panels/IEditorPanel.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/ScrollView.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Image.h"
#include <ostream>
#include <streambuf>
#include <string>
#include <vector>
#include <functional>

namespace Editor {

class LogCaptureBuf : public std::streambuf {
  public:
	using LineCallback = std::function<void(std::string)>;
	void SetCallback(LineCallback cb) { m_Callback = std::move(cb); }

  protected:
	int overflow(int c) override;
	std::streamsize xsputn(const char *s, std::streamsize n) override;

  private:
	std::string m_Line;
	LineCallback m_Callback;
};

enum class LogLevel { Trace, Debug, Info, Warning, Error, Critical };

class ConsolePanel : public IEditorPanel {
  public:
	explicit ConsolePanel(Aquila::UI::Core::TextureCache *textureCache);
	~ConsolePanel();

	void Build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlayRoot) override;
	void FlushPending();
	void ClearAll();

  private:
	struct LogEntry {
		LogLevel level;
		std::string message;
	};

	enum class FilterGroup { Info, Warning, Error };

	void AppendEntry(LogEntry entry);
	void SelectRow(int index);
	void ToggleFilter(FilterGroup group);
	void ApplyRowVisibility(int index);
	void UpdateFilterButtons();
	static LogLevel ParseLevel(const std::string &line);
	static FilterGroup LevelToGroup(LogLevel level);
	static const char *LevelClass(LogLevel level);
	static const char *LevelIconClass(LogLevel level);
	static vec4 LevelIconTint(LogLevel level);
	Aquila::GFX::GfxTexture *LevelIcon(LogLevel level) const;
	static constexpr int kMaxMessages = 500;

	Aquila::UI::Core::TextureCache *m_TextureCache = nullptr;
	Aquila::GFX::GfxTexture *m_InfoIcon = nullptr;
	Aquila::GFX::GfxTexture *m_AlertIcon = nullptr;
	Aquila::GFX::GfxTexture *m_ErrorIcon = nullptr;

	LogCaptureBuf m_CaptureBuf;
	std::ostream m_CaptureStream{ &m_CaptureBuf };

	std::vector<std::string> m_Pending;
	std::vector<LogEntry> m_Entries;

	int m_InfoCount = 0;
	int m_WarningCount = 0;
	int m_ErrorCount = 0;
	bool m_ShowInfo = true;
	bool m_ShowWarning = true;
	bool m_ShowError = true;
	int m_SelectedIndex = -1;

	Aquila::UI::Core::ScrollView *m_ScrollView = nullptr;
	Aquila::UI::Core::Label *m_DetailLabel = nullptr;
	std::vector<Aquila::UI::Core::View *> m_Rows;

	Aquila::UI::Core::Label *m_InfoCountLabel = nullptr;
	Aquila::UI::Core::Label *m_WarningCountLabel = nullptr;
	Aquila::UI::Core::Label *m_ErrorCountLabel = nullptr;

	Aquila::UI::Core::View *m_InfoFilterBtn = nullptr;
	Aquila::UI::Core::View *m_WarningFilterBtn = nullptr;
	Aquila::UI::Core::View *m_ErrorFilterBtn = nullptr;
};

} // namespace Editor
