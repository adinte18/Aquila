#include "UI/Panels/ConsolePanel.h"

#include "Aquila/Foundation/Log.h"
#include "Aquila/UI/Style/StyleProperties.h"
#include "Aquila/GFX/GfxTexture.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::Foundation;
using namespace Aquila::GFX;
using namespace Aquila::UI;

namespace {

class ClickableView : public UI::Core::View {
  public:
	Signal<void()> on_click;

	void on_mouse_release(Platform::MouseButton btn, Vec2 pos) override {
		View::on_mouse_release(btn, pos);
		if (btn == Platform::MouseButton::Left) {
			on_click();
		}
	}
};

} // namespace

int LogCaptureBuf::overflow(int c) {
	if (c == traits_type::eof()) {
		return traits_type::eof();
	}
	if (static_cast<char>(c) == '\n') {
		if (m_callback && !m_line.empty()) {
			m_callback(m_line);
		}
		m_line.clear();
	} else {
		m_line += static_cast<char>(c);
	}
	return c;
}

std::streamsize LogCaptureBuf::xsputn(const char *s, std::streamsize n) {
	for (std::streamsize i = 0; i < n; ++i) {
		overflow(static_cast<unsigned char>(s[i]));
	}
	return n;
}

ConsolePanel::ConsolePanel(UI::Core::TextureCache *texture_cache) : m_texture_cache(texture_cache) {
	m_capture_buf.set_callback([this](std::string line) { m_pending.push_back(std::move(line)); });
	Logger::enable_colors(false);
	Logger::set_sink(&m_capture_stream);
}

ConsolePanel::~ConsolePanel() {
	Logger::set_sink(nullptr);
}

void ConsolePanel::build(UI::Core::DockPanel *panel, UI::Core::View * /*overlayRoot*/) {
	if (m_texture_cache) {
		m_info_icon = m_texture_cache->load("Engine/UI/Icons/info.png");
		m_alert_icon = m_texture_cache->load("Engine/UI/Icons/triangle-alert.png");
		m_error_icon = m_texture_cache->load("Engine/UI/Icons/circle-x.png");
	}

	m_scroll_view = panel->find_by_id<UI::Core::ScrollView>("console-scroll");
	m_detail_label = panel->find_by_id<UI::Core::Label>("console-detail-text");

	UI::Core::View *toolbar_ptr = panel->find_by_id("console-toolbar");
	if (toolbar_ptr == nullptr) {
		return;
	}

	auto make_filter_btn = [&](Vec4 icon_tint, FilterGroup group, UI::Core::View *&btn_out,
							   UI::Core::Label *&count_out) {
		auto btn = create_unique<ClickableView>();
		btn->add_class("console-filter-btn");
		btn_out = btn.get();

		GfxTexture *icon_tex = nullptr;
		switch (group) {
		case FilterGroup::Info:
			icon_tex = m_info_icon;
			break;
		case FilterGroup::Warning:
			icon_tex = m_alert_icon;
			break;
		case FilterGroup::Error:
			icon_tex = m_error_icon;
			break;
		}
		auto icon = create_unique<UI::Core::Image>(icon_tex, icon_tint);
		icon->add_class("console-filter-icon");
		btn->add_child(std::move(icon));

		auto count = create_unique<UI::Core::Label>("0");
		count->add_class("console-filter-count");
		count_out = static_cast<UI::Core::Label *>(btn->add_child(std::move(count)));

		btn->on_click.connect([this, group]() { toggle_filter(group); });
		toolbar_ptr->add_child(std::move(btn));
	};

	make_filter_btn({ 0.42f, 0.69f, 0.86f, 1.F }, FilterGroup::Info, m_info_filter_btn, m_info_count_label);
	make_filter_btn({ 0.83f, 0.67f, 0.29f, 1.F }, FilterGroup::Warning, m_warning_filter_btn, m_warning_count_label);
	make_filter_btn({ 0.83f, 0.42f, 0.42f, 1.F }, FilterGroup::Error, m_error_filter_btn, m_error_count_label);
}

void ConsolePanel::flush_pending() {
	if (m_pending.empty() || !m_scroll_view) {
		return;
	}
	for (auto &line : m_pending) {
		LogLevel level = parse_level(line);
		append_entry({ level, std::move(line) });
	}
	m_pending.clear();
	update_filter_buttons();
}

void ConsolePanel::append_entry(LogEntry entry) {
	if ((int)m_entries.size() >= K_MAX_MESSAGES) {
		auto &oldest = m_entries.front();
		switch (level_to_group(oldest.level)) {
		case FilterGroup::Info:
			--m_info_count;
			break;
		case FilterGroup::Warning:
			--m_warning_count;
			break;
		case FilterGroup::Error:
			--m_error_count;
			break;
		}
		m_entries.erase(m_entries.begin());
		m_rows.erase(m_rows.begin());
		if (m_selected_index == 0) {
			m_selected_index = -1;
			if (m_detail_label) {
				m_detail_label->set_text("");
			}
		} else if (m_selected_index > 0) {
			--m_selected_index;
		}
		m_scroll_view->remove_oldest_content();
	}

	int row_index = (int)m_entries.size();

	switch (level_to_group(entry.level)) {
	case FilterGroup::Info:
		++m_info_count;
		break;
	case FilterGroup::Warning:
		++m_warning_count;
		break;
	case FilterGroup::Error:
		++m_error_count;
		break;
	}

	auto row = create_unique<ClickableView>();
	row->add_class("console-row");
	row->add_class(level_class(entry.level));
	row->add_class((row_index % 2 == 0) ? "console-row-even" : "console-row-odd");

	auto icon = create_unique<UI::Core::Image>(level_icon(entry.level), level_icon_tint(entry.level));
	icon->add_class("console-row-icon");
	row->add_child(std::move(icon));

	auto text = create_unique<UI::Core::Label>(entry.message);
	text->add_class("console-row-text");
	row->add_child(std::move(text));

	ClickableView *row_ptr = static_cast<ClickableView *>(m_scroll_view->add_content(std::move(row)));
	row_ptr->on_click.connect([this, row_ptr]() {
		auto it = std::find(m_rows.begin(), m_rows.end(), static_cast<UI::Core::View *>(row_ptr));
		if (it != m_rows.end()) {
			select_row((int)(it - m_rows.begin()));
		}
	});

	m_rows.push_back(row_ptr);
	m_entries.push_back(std::move(entry));

	apply_row_visibility(row_index);
}

void ConsolePanel::select_row(int index) {
	if (m_selected_index >= 0 && m_selected_index < (int)m_rows.size()) {
		m_rows[m_selected_index]->remove_class("console-row-selected");
	}
	m_selected_index = index;
	if (index >= 0 && index < (int)m_rows.size()) {
		m_rows[index]->add_class("console-row-selected");
		if (m_detail_label) {
			m_detail_label->set_text(m_entries[index].message);
		}
	}
}

void ConsolePanel::clear_all() {
	while (!m_rows.empty()) {
		m_scroll_view->remove_oldest_content();
		m_rows.erase(m_rows.begin());
	}
	m_entries.clear();
	m_info_count = 0;
	m_warning_count = 0;
	m_error_count = 0;
	m_selected_index = -1;
	if (m_detail_label) {
		m_detail_label->set_text("");
	}
	update_filter_buttons();
}

void ConsolePanel::toggle_filter(FilterGroup group) {
	switch (group) {
	case FilterGroup::Info:
		m_show_info = !m_show_info;
		if (m_info_filter_btn) {
			m_info_filter_btn->set_class("dimmed", !m_show_info);
		}
		break;
	case FilterGroup::Warning:
		m_show_warning = !m_show_warning;
		if (m_warning_filter_btn) {
			m_warning_filter_btn->set_class("dimmed", !m_show_warning);
		}
		break;
	case FilterGroup::Error:
		m_show_error = !m_show_error;
		if (m_error_filter_btn) {
			m_error_filter_btn->set_class("dimmed", !m_show_error);
		}
		break;
	}
	for (int i = 0; i < (int)m_rows.size(); ++i) {
		apply_row_visibility(i);
	}
}

void ConsolePanel::apply_row_visibility(int index) {
	if (index < 0 || index >= (int)m_rows.size()) {
		return;
	}
	FilterGroup group = level_to_group(m_entries[index].level);
	bool visible = false;
	switch (group) {
	case FilterGroup::Info:
		visible = m_show_info;
		break;
	case FilterGroup::Warning:
		visible = m_show_warning;
		break;
	case FilterGroup::Error:
		visible = m_show_error;
		break;
	}
	m_rows[index]->set_hidden(!visible);
}

void ConsolePanel::update_filter_buttons() {
	if (m_info_count_label) {
		m_info_count_label->set_text(std::to_string(m_info_count));
	}
	if (m_warning_count_label) {
		m_warning_count_label->set_text(std::to_string(m_warning_count));
	}
	if (m_error_count_label) {
		m_error_count_label->set_text(std::to_string(m_error_count));
	}
}

LogLevel ConsolePanel::parse_level(const std::string &line) {
	if (line.find("[AQUILA CRITICAL]") != std::string::npos) {
		return LogLevel::Critical;
	}
	if (line.find("[AQUILA ERROR]") != std::string::npos) {
		return LogLevel::Error;
	}
	if (line.find("[AQUILA WARNING]") != std::string::npos) {
		return LogLevel::Warning;
	}
	if (line.find("[AQUILA DEBUG]") != std::string::npos) {
		return LogLevel::Debug;
	}
	if (line.find("[AQUILA TRACE]") != std::string::npos) {
		return LogLevel::Trace;
	}
	return LogLevel::Info;
}

ConsolePanel::FilterGroup ConsolePanel::level_to_group(LogLevel level) {
	switch (level) {
	case LogLevel::Warning:
		return FilterGroup::Warning;
	case LogLevel::Error:
		return FilterGroup::Error;
	case LogLevel::Critical:
		return FilterGroup::Error;
	default:
		return FilterGroup::Info;
	}
}

const char *ConsolePanel::level_class(LogLevel level) {
	switch (level) {
	case LogLevel::Critical:
		return "console-critical";
	case LogLevel::Error:
		return "console-error";
	case LogLevel::Warning:
		return "console-warning";
	case LogLevel::Debug:
		return "console-debug";
	case LogLevel::Trace:
		return "console-trace";
	default:
		return "console-info";
	}
}

const char *ConsolePanel::level_icon_class(LogLevel level) {
	switch (level) {
	case LogLevel::Critical:
		return "console-icon-critical";
	case LogLevel::Error:
		return "console-icon-error";
	case LogLevel::Warning:
		return "console-icon-warning";
	case LogLevel::Debug:
		return "console-icon-debug";
	case LogLevel::Trace:
		return "console-icon-trace";
	default:
		return "console-icon-info";
	}
}

Vec4 ConsolePanel::level_icon_tint(LogLevel level) {
	switch (level) {
	case LogLevel::Warning:
		return { 0.83f, 0.67f, 0.29f, 1.F };
	case LogLevel::Error:
		return { 0.83f, 0.42f, 0.42f, 1.F };
	case LogLevel::Critical:
		return { 1.F, 0.25f, 0.25f, 1.F };
	case LogLevel::Debug:
		return { 0.42f, 0.69f, 0.86f, 1.F };
	case LogLevel::Trace:
		return { 0.53f, 0.53f, 0.53f, 1.F };
	default:
		return { 0.42f, 0.69f, 0.86f, 1.F };
	}
}

GfxTexture *ConsolePanel::level_icon(LogLevel level) const {
	switch (level_to_group(level)) {
	case FilterGroup::Warning:
		return m_alert_icon;
	case FilterGroup::Error:
		return m_error_icon;
	default:
		return m_info_icon;
	}
}

} // namespace Editor
