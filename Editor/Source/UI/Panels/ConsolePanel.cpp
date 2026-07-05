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
	Signal<void()> onClick;

	void OnMouseRelease(Platform::MouseButton btn, vec2 pos) override {
		View::OnMouseRelease(btn, pos);
		if (btn == Platform::MouseButton::Left) {
			onClick();
		}
	}
};

} // namespace

int LogCaptureBuf::overflow(int c) {
	if (c == traits_type::eof()) {
		return traits_type::eof();
	}
	if (static_cast<char>(c) == '\n') {
		if (m_Callback && !m_Line.empty()) {
			m_Callback(m_Line);
		}
		m_Line.clear();
	} else {
		m_Line += static_cast<char>(c);
	}
	return c;
}

std::streamsize LogCaptureBuf::xsputn(const char *s, std::streamsize n) {
	for (std::streamsize i = 0; i < n; ++i) {
		overflow(static_cast<unsigned char>(s[i]));
	}
	return n;
}

ConsolePanel::ConsolePanel(UI::Core::TextureCache *textureCache) : m_TextureCache(textureCache) {
	m_CaptureBuf.SetCallback([this](std::string line) { m_Pending.push_back(std::move(line)); });
	Logger::EnableColors(false);
	Logger::SetSink(&m_CaptureStream);
}

ConsolePanel::~ConsolePanel() {
	Logger::SetSink(nullptr);
}

void ConsolePanel::Build(UI::Core::DockPanel *panel, UI::Core::View * /*overlayRoot*/) {
	if (m_TextureCache) {
		m_InfoIcon = m_TextureCache->Load("Engine/UI/Icons/info.png");
		m_AlertIcon = m_TextureCache->Load("Engine/UI/Icons/triangle-alert.png");
		m_ErrorIcon = m_TextureCache->Load("Engine/UI/Icons/circle-x.png");
	}

	m_ScrollView = panel->FindById<UI::Core::ScrollView>("console-scroll");
	m_DetailLabel = panel->FindById<UI::Core::Label>("console-detail-text");

	UI::Core::View *toolbarPtr = panel->FindById("console-toolbar");
	if (toolbarPtr == nullptr) {
		return;
	}

	auto makeFilterBtn = [&](vec4 iconTint, FilterGroup group, UI::Core::View *&btnOut, UI::Core::Label *&countOut) {
		auto btn = CreateUnique<ClickableView>();
		btn->AddClass("console-filter-btn");
		btnOut = btn.get();

		GfxTexture *iconTex = nullptr;
		switch (group) {
		case FilterGroup::Info:
			iconTex = m_InfoIcon;
			break;
		case FilterGroup::Warning:
			iconTex = m_AlertIcon;
			break;
		case FilterGroup::Error:
			iconTex = m_ErrorIcon;
			break;
		}
		auto icon = CreateUnique<UI::Core::Image>(iconTex, iconTint);
		icon->AddClass("console-filter-icon");
		btn->AddChild(std::move(icon));

		auto count = CreateUnique<UI::Core::Label>("0");
		count->AddClass("console-filter-count");
		countOut = static_cast<UI::Core::Label *>(btn->AddChild(std::move(count)));

		btn->onClick.Connect([this, group]() { ToggleFilter(group); });
		toolbarPtr->AddChild(std::move(btn));
	};

	makeFilterBtn({ 0.42f, 0.69f, 0.86f, 1.f }, FilterGroup::Info, m_InfoFilterBtn, m_InfoCountLabel);
	makeFilterBtn({ 0.83f, 0.67f, 0.29f, 1.f }, FilterGroup::Warning, m_WarningFilterBtn, m_WarningCountLabel);
	makeFilterBtn({ 0.83f, 0.42f, 0.42f, 1.f }, FilterGroup::Error, m_ErrorFilterBtn, m_ErrorCountLabel);
}

void ConsolePanel::FlushPending() {
	if (m_Pending.empty() || !m_ScrollView) {
		return;
	}
	for (auto &line : m_Pending) {
		LogLevel level = ParseLevel(line);
		AppendEntry({ level, std::move(line) });
	}
	m_Pending.clear();
	UpdateFilterButtons();
}

void ConsolePanel::AppendEntry(LogEntry entry) {
	if ((int)m_Entries.size() >= kMaxMessages) {
		auto &oldest = m_Entries.front();
		switch (LevelToGroup(oldest.level)) {
		case FilterGroup::Info:
			--m_InfoCount;
			break;
		case FilterGroup::Warning:
			--m_WarningCount;
			break;
		case FilterGroup::Error:
			--m_ErrorCount;
			break;
		}
		m_Entries.erase(m_Entries.begin());
		m_Rows.erase(m_Rows.begin());
		if (m_SelectedIndex == 0) {
			m_SelectedIndex = -1;
			if (m_DetailLabel) {
				m_DetailLabel->SetText("");
			}
		} else if (m_SelectedIndex > 0) {
			--m_SelectedIndex;
		}
		m_ScrollView->RemoveOldestContent();
	}

	int rowIndex = (int)m_Entries.size();

	switch (LevelToGroup(entry.level)) {
	case FilterGroup::Info:
		++m_InfoCount;
		break;
	case FilterGroup::Warning:
		++m_WarningCount;
		break;
	case FilterGroup::Error:
		++m_ErrorCount;
		break;
	}

	auto row = CreateUnique<ClickableView>();
	row->AddClass("console-row");
	row->AddClass(LevelClass(entry.level));
	row->AddClass((rowIndex % 2 == 0) ? "console-row-even" : "console-row-odd");

	auto icon = CreateUnique<UI::Core::Image>(LevelIcon(entry.level), LevelIconTint(entry.level));
	icon->AddClass("console-row-icon");
	row->AddChild(std::move(icon));

	auto text = CreateUnique<UI::Core::Label>(entry.message);
	text->AddClass("console-row-text");
	row->AddChild(std::move(text));

	ClickableView *rowPtr = static_cast<ClickableView *>(m_ScrollView->AddContent(std::move(row)));
	rowPtr->onClick.Connect([this, rowPtr]() {
		auto it = std::find(m_Rows.begin(), m_Rows.end(), static_cast<UI::Core::View *>(rowPtr));
		if (it != m_Rows.end()) {
			SelectRow((int)(it - m_Rows.begin()));
		}
	});

	m_Rows.push_back(rowPtr);
	m_Entries.push_back(std::move(entry));

	ApplyRowVisibility(rowIndex);
}

void ConsolePanel::SelectRow(int index) {
	if (m_SelectedIndex >= 0 && m_SelectedIndex < (int)m_Rows.size()) {
		m_Rows[m_SelectedIndex]->RemoveClass("console-row-selected");
	}
	m_SelectedIndex = index;
	if (index >= 0 && index < (int)m_Rows.size()) {
		m_Rows[index]->AddClass("console-row-selected");
		if (m_DetailLabel) {
			m_DetailLabel->SetText(m_Entries[index].message);
		}
	}
}

void ConsolePanel::ClearAll() {
	while (!m_Rows.empty()) {
		m_ScrollView->RemoveOldestContent();
		m_Rows.erase(m_Rows.begin());
	}
	m_Entries.clear();
	m_InfoCount = 0;
	m_WarningCount = 0;
	m_ErrorCount = 0;
	m_SelectedIndex = -1;
	if (m_DetailLabel) {
		m_DetailLabel->SetText("");
	}
	UpdateFilterButtons();
}

void ConsolePanel::ToggleFilter(FilterGroup group) {
	switch (group) {
	case FilterGroup::Info:
		m_ShowInfo = !m_ShowInfo;
		if (m_InfoFilterBtn) {
			m_InfoFilterBtn->SetClass("dimmed", !m_ShowInfo);
		}
		break;
	case FilterGroup::Warning:
		m_ShowWarning = !m_ShowWarning;
		if (m_WarningFilterBtn) {
			m_WarningFilterBtn->SetClass("dimmed", !m_ShowWarning);
		}
		break;
	case FilterGroup::Error:
		m_ShowError = !m_ShowError;
		if (m_ErrorFilterBtn) {
			m_ErrorFilterBtn->SetClass("dimmed", !m_ShowError);
		}
		break;
	}
	for (int i = 0; i < (int)m_Rows.size(); ++i) {
		ApplyRowVisibility(i);
	}
}

void ConsolePanel::ApplyRowVisibility(int index) {
	if (index < 0 || index >= (int)m_Rows.size()) {
		return;
	}
	FilterGroup group = LevelToGroup(m_Entries[index].level);
	bool visible = false;
	switch (group) {
	case FilterGroup::Info:
		visible = m_ShowInfo;
		break;
	case FilterGroup::Warning:
		visible = m_ShowWarning;
		break;
	case FilterGroup::Error:
		visible = m_ShowError;
		break;
	}
	m_Rows[index]->SetHidden(!visible);
}

void ConsolePanel::UpdateFilterButtons() {
	if (m_InfoCountLabel) {
		m_InfoCountLabel->SetText(std::to_string(m_InfoCount));
	}
	if (m_WarningCountLabel) {
		m_WarningCountLabel->SetText(std::to_string(m_WarningCount));
	}
	if (m_ErrorCountLabel) {
		m_ErrorCountLabel->SetText(std::to_string(m_ErrorCount));
	}
}

LogLevel ConsolePanel::ParseLevel(const std::string &line) {
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

ConsolePanel::FilterGroup ConsolePanel::LevelToGroup(LogLevel level) {
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

const char *ConsolePanel::LevelClass(LogLevel level) {
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

const char *ConsolePanel::LevelIconClass(LogLevel level) {
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

vec4 ConsolePanel::LevelIconTint(LogLevel level) {
	switch (level) {
	case LogLevel::Warning:
		return { 0.83f, 0.67f, 0.29f, 1.f };
	case LogLevel::Error:
		return { 0.83f, 0.42f, 0.42f, 1.f };
	case LogLevel::Critical:
		return { 1.f, 0.25f, 0.25f, 1.f };
	case LogLevel::Debug:
		return { 0.42f, 0.69f, 0.86f, 1.f };
	case LogLevel::Trace:
		return { 0.53f, 0.53f, 0.53f, 1.f };
	default:
		return { 0.42f, 0.69f, 0.86f, 1.f };
	}
}

GfxTexture *ConsolePanel::LevelIcon(LogLevel level) const {
	switch (LevelToGroup(level)) {
	case FilterGroup::Warning:
		return m_AlertIcon;
	case FilterGroup::Error:
		return m_ErrorIcon;
	default:
		return m_InfoIcon;
	}
}

} // namespace Editor
