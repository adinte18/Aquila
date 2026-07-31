#include "UI/Windows/SettingsWindow.h"

#include "Aquila/Application/Events/Event.h"
#include "Aquila/Application/Events/WindowEvent.h"
#include "Aquila/UI/Core/Canvas.h"
#include "Aquila/UI/Core/TextureCache.h"
#include "Aquila/UI/Core/View.h"
#include "Aquila/UI/Style/StyleParser.h"

#include "Aquila/UI/Widgets/Button.h"
#include "Aquila/UI/Widgets/Collapsible.h"
#include "Aquila/UI/Widgets/DragFloat.h"
#include "Aquila/UI/Widgets/Dropdown.h"
#include "Aquila/UI/Widgets/Image.h"
#include "Aquila/UI/Widgets/Label.h"
#include "Aquila/UI/Widgets/ScrollView.h"

namespace Editor {

using namespace Aquila;
using namespace Aquila::UI::Core;

namespace {

struct FontFamily {
	const char *id;
	const char *label;
};

constexpr FontFamily k_font_families[] = {
	{ "Lexend", "Lexend" },
	{ "Inconsolata", "Inconsolata" },
};

} // namespace

SettingsWindow::SettingsWindow() = default;
SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::build(TextureCache *texture_cache, Uint32 width, Uint32 height, const std::string &style_path) {
	m_texture_cache = texture_cache;
	if (m_texture_cache) {
		m_help_icon = m_texture_cache->load("Engine/UI/Icons/circle-question-mark.png");
	}

	m_canvas = std::make_unique<Canvas>(width, height);
	UI::StyleParser::load_file(style_path, m_canvas->get_style_sheet());

	m_working = Config::get_preferences();

	auto *root = m_canvas->get_root();
	root->set_id("settings-root");
	root->add_class("settings-root");

	auto *header = root->add_child<View>();
	header->add_class("settings-header");
	auto *title = header->add_child<Label>(std::string("Editor Settings"));
	title->add_class("settings-title");

	auto *scroll = root->add_child<ScrollView>();
	scroll->add_class("settings-scroll");
	auto *content = scroll->add_child<View>();
	content->add_class("settings-content");

	auto *interface_section = add_section(content, "Interface");
	font_row(interface_section, "Main font", "The primary interface font used for all editor text.",
			 &m_working.fonts.main_family);
	font_row(interface_section, "Mono font", "Monospace font used by the console and code-style text.",
			 &m_working.fonts.mono_family);
	float_row(interface_section, "Interface scale", "Scales the size of all interface text. 1.0 is the default.",
			  &m_working.ui_scale, 0.75F, 1.75F, 0.01F, 2);

	auto *footer = root->add_child<View>();
	footer->add_class("settings-footer");
	auto *spacer = footer->add_child<View>();
	spacer->add_class("settings-footer-spacer");

	auto *reset_btn = footer->add_child<Button>(std::string("Reset"));
	reset_btn->add_class("settings-btn");
	reset_btn->on_click.connect([this] { reset(); });

	auto *apply_btn = footer->add_child<Button>(std::string("Apply"));
	apply_btn->add_class("settings-btn");
	apply_btn->on_click.connect([this] { apply(); });

	auto *save_btn = footer->add_child<Button>(std::string("Save"));
	save_btn->add_class("settings-btn");
	save_btn->add_class("settings-btn-primary");
	save_btn->on_click.connect([this] {
		apply();
		Config::get_preferences().save_to_file();
	});

	auto *close_btn = footer->add_child<Button>(std::string("Close"));
	close_btn->add_class("settings-btn");
	close_btn->on_click.connect([this] {
		if (on_request_close) {
			on_request_close();
		}
	});

	m_canvas->reload_styles();
}

View *SettingsWindow::add_section(View *host, const std::string &title) {
	return host->add_child<Collapsible>(title);
}

View *SettingsWindow::make_row(View *host, const std::string &label, const std::string &description) {
	auto *row = host->add_child<View>();
	row->add_class("settings-row");

	auto *lbl = row->add_child<Label>(label);
	lbl->add_class("settings-label");

	auto *help = row->add_child<View>();
	help->add_class("settings-help");
	help->set_tooltip(description);
	auto *icon = help->add_child<UI::Core::Image>(m_help_icon, Vec4(1.F));
	icon->add_class("settings-help-icon");

	return row;
}

void SettingsWindow::float_row(View *host, const std::string &label, const std::string &description, float *field,
							   float min, float max, float speed, int precision) {
	auto *row = make_row(host, label, description);

	DragFloat::Config config;
	config.min = min;
	config.max = max;
	config.speed = speed;
	config.precision = precision;

	auto *drag = row->add_child<DragFloat>(config);
	drag->add_class("settings-field");
	drag->set_value(*field);
	drag->on_changed.connect([field](float value) { *field = value; });
	m_sync.push_back([drag, field] { drag->set_value(*field); });
}

void SettingsWindow::font_row(View *host, const std::string &label, const std::string &description,
							  std::string *family_field) {
	auto *row = make_row(host, label, description);

	auto *dropdown = row->add_child<Dropdown>();
	dropdown->add_class("settings-field");
	for (const auto &family : k_font_families) {
		dropdown->add_option(family.id, family.label);
	}
	dropdown->set_value(*family_field);
	dropdown->on_changed.connect([family_field](const std::string &id) { *family_field = id; });
	m_sync.push_back([dropdown, family_field] { dropdown->set_value(*family_field); });
}

void SettingsWindow::apply() {
	Config::get_preferences() = m_working;
	if (on_applied) {
		on_applied();
	}
	m_canvas->reload_styles();
}

void SettingsWindow::reset() {
	m_working.reset_to_defaults();
	sync_widgets();
	apply();
}

void SettingsWindow::sync_widgets() {
	for (auto &fn : m_sync) {
		fn();
	}
}

void SettingsWindow::update(F32 delta_time) {
	m_canvas->update(delta_time);
	m_canvas->compute();
}

void SettingsWindow::render(Graphics::QuadBatcher &batcher, GFX::GfxCommandList &cmd) {
	m_canvas->submit_to_quad_batcher(batcher, cmd);
}

void SettingsWindow::on_event(Application::Events::Event &event) {
	Application::Events::EventDispatcher dispatcher(event);
	dispatcher.dispatch<Application::Events::WindowResizeEvent>([this](Application::Events::WindowResizeEvent &e) {
		if (e.get_width() > 0 && e.get_height() > 0) {
			m_canvas->resize(e.get_width(), e.get_height());
		}
		return false;
	});

	m_canvas->on_event(event);
}

} // namespace Editor
