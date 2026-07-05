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
	void set_callback(LineCallback cb) { m_callback = std::move(cb); }

  protected:
	int overflow(int c) override;
	std::streamsize xsputn(const char *s, std::streamsize n) override;

  private:
	std::string m_line;
	LineCallback m_callback;
};

enum class LogLevel { Trace, Debug, Info, Warning, Error, Critical };

class ConsolePanel : public IEditorPanel {
  public:
	explicit ConsolePanel(Aquila::UI::Core::TextureCache *texture_cache);
	~ConsolePanel();

	void build(Aquila::UI::Core::DockPanel *panel, Aquila::UI::Core::View *overlay_root) override;
	void flush_pending();
	void clear_all();

  private:
	struct LogEntry {
		LogLevel level;
		std::string message;
	};

	enum class FilterGroup { Info, Warning, Error };

	void append_entry(LogEntry entry);
	void select_row(int index);
	void toggle_filter(FilterGroup group);
	void apply_row_visibility(int index);
	void update_filter_buttons();
	static LogLevel parse_level(const std::string &line);
	static FilterGroup level_to_group(LogLevel level);
	static const char *level_class(LogLevel level);
	static const char *level_icon_class(LogLevel level);
	static Vec4 level_icon_tint(LogLevel level);
	Aquila::GFX::GfxTexture *level_icon(LogLevel level) const;
	static constexpr int K_MAX_MESSAGES = 500;

	Aquila::UI::Core::TextureCache *m_texture_cache = nullptr;
	Aquila::GFX::GfxTexture *m_info_icon = nullptr;
	Aquila::GFX::GfxTexture *m_alert_icon = nullptr;
	Aquila::GFX::GfxTexture *m_error_icon = nullptr;

	LogCaptureBuf m_capture_buf;
	std::ostream m_capture_stream{ &m_capture_buf };

	std::vector<std::string> m_pending;
	std::vector<LogEntry> m_entries;

	int m_info_count = 0;
	int m_warning_count = 0;
	int m_error_count = 0;
	bool m_show_info = true;
	bool m_show_warning = true;
	bool m_show_error = true;
	int m_selected_index = -1;

	Aquila::UI::Core::ScrollView *m_scroll_view = nullptr;
	Aquila::UI::Core::Label *m_detail_label = nullptr;
	std::vector<Aquila::UI::Core::View *> m_rows;

	Aquila::UI::Core::Label *m_info_count_label = nullptr;
	Aquila::UI::Core::Label *m_warning_count_label = nullptr;
	Aquila::UI::Core::Label *m_error_count_label = nullptr;

	Aquila::UI::Core::View *m_info_filter_btn = nullptr;
	Aquila::UI::Core::View *m_warning_filter_btn = nullptr;
	Aquila::UI::Core::View *m_error_filter_btn = nullptr;
};

} // namespace Editor
