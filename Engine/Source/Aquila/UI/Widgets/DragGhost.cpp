#include "Aquila/UI/Widgets/DragGhost.h"
#include "Aquila/UI/Style/StyleTypes.h"

namespace Aquila::UI::Core {

DragGhost::DragGhost() {
	add_class("drag-ghost");
	set_hidden(true);
}

void DragGhost::show(std::string label, Vec2 pos, GFX::GfxTexture *icon) {
	if (label.empty()) {
		hide();
		return;
	}
	set_text(std::move(label));
	set_icon_texture(icon);
	set_hidden(false);
	set_position(pos);
}

void DragGhost::move_to(Vec2 pos) {
	if (!is_visible()) {
		return;
	}
	set_position(pos);
}

void DragGhost::hide() {
	set_hidden(true);
}

void DragGhost::set_position(Vec2 pos) {
	FloatingConfig cfg;
	cfg.attach_to = FloatingAttachTo::Root;
	cfg.element_point = FloatingAttachPoint::LeftTop;
	cfg.parent_point = FloatingAttachPoint::LeftTop;
	cfg.z_index = 100;
	cfg.offset = pos + Vec2(14.F, 14.F);
	set_floating(cfg);
	invalidate_layout();
}

} // namespace Aquila::UI::Core
