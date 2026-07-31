#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include "Aquila/UI/Core/DockLayoutSerializer.h"
#include "Aquila/UI/Widgets/DockNode.h"
#include "Aquila/UI/Widgets/DockPanel.h"
#include "Aquila/UI/Widgets/DockSpace.h"

using namespace Aquila::UI::Core;

namespace {

DockLayoutDesc make_sample() {
	DockLayoutDesc desc;
	desc.root.is_leaf = false;
	desc.root.direction = 1;
	desc.root.fraction = 1.F;

	DockNodeDesc top;
	top.is_leaf = false;
	top.direction = 0;
	top.fraction = 0.75F;

	DockNodeDesc left;
	left.is_leaf = true;
	left.fraction = 0.25F;
	left.active_index = 0;
	left.panel_ids = { "panel-hierarchy" };

	DockNodeDesc center;
	center.is_leaf = true;
	center.fraction = 0.5F;
	center.active_index = 1;
	center.panel_ids = { "panel-viewport", "panel-inspector" };

	DockNodeDesc right;
	right.is_leaf = true;
	right.fraction = 0.25F;
	right.active_index = -1;
	right.panel_ids = {};

	top.children = { left, center, right };

	DockNodeDesc bottom;
	bottom.is_leaf = true;
	bottom.fraction = 0.25F;
	bottom.active_index = 0;
	bottom.panel_ids = { "panel-console" };

	desc.root.children = { top, bottom };
	return desc;
}

void check_equal(const DockNodeDesc &a, const DockNodeDesc &b) {
	CHECK(a.is_leaf == b.is_leaf);
	CHECK(a.fraction == doctest::Approx(b.fraction));
	if (a.is_leaf) {
		CHECK(a.active_index == b.active_index);
		REQUIRE(a.panel_ids.size() == b.panel_ids.size());
		for (size_t i = 0; i < a.panel_ids.size(); ++i) {
			CHECK(a.panel_ids[i] == b.panel_ids[i]);
		}
		return;
	}
	CHECK(a.direction == b.direction);
	REQUIRE(a.children.size() == b.children.size());
	for (size_t i = 0; i < a.children.size(); ++i) {
		check_equal(a.children[i], b.children[i]);
	}
}

void check_structure(const DockNodeDesc &a, const DockNodeDesc &b) {
	CHECK(a.is_leaf == b.is_leaf);
	if (a.is_leaf) {
		CHECK(a.active_index == b.active_index);
		REQUIRE(a.panel_ids.size() == b.panel_ids.size());
		for (size_t i = 0; i < a.panel_ids.size(); ++i) {
			CHECK(a.panel_ids[i] == b.panel_ids[i]);
		}
		return;
	}
	CHECK(a.direction == b.direction);
	REQUIRE(a.children.size() == b.children.size());
	for (size_t i = 0; i < a.children.size(); ++i) {
		check_structure(a.children[i], b.children[i]);
	}
}

} // namespace

TEST_SUITE("DockLayoutSerializer") {
	TEST_CASE("encode then decode round-trips the tree") {
		const DockLayoutDesc original = make_sample();
		const std::vector<Uint8> bytes = DockLayoutSerializer::encode(original);
		const Option<DockLayoutDesc> restored = DockLayoutSerializer::decode(bytes.data(), bytes.size());
		REQUIRE(restored.has_value());
		check_equal(original.root, restored->root);
	}

	TEST_CASE("stream begins with the AQDL magic and version 1") {
		const std::vector<Uint8> bytes = DockLayoutSerializer::encode(make_sample());
		REQUIRE(bytes.size() >= 8);
		CHECK(bytes[0] == 'A');
		CHECK(bytes[1] == 'Q');
		CHECK(bytes[2] == 'D');
		CHECK(bytes[3] == 'L');
		const Uint32 version = static_cast<Uint32>(bytes[4]) | (static_cast<Uint32>(bytes[5]) << 8) |
							   (static_cast<Uint32>(bytes[6]) << 16) | (static_cast<Uint32>(bytes[7]) << 24);
		CHECK(version == 1u);
	}

	TEST_CASE("decode rejects a bad magic word") {
		std::vector<Uint8> bytes = DockLayoutSerializer::encode(make_sample());
		bytes[0] = 'X';
		CHECK_FALSE(DockLayoutSerializer::decode(bytes.data(), bytes.size()).has_value());
	}

	TEST_CASE("decode rejects a truncated stream") {
		const std::vector<Uint8> bytes = DockLayoutSerializer::encode(make_sample());
		CHECK_FALSE(DockLayoutSerializer::decode(bytes.data(), bytes.size() / 2).has_value());
		CHECK_FALSE(DockLayoutSerializer::decode(bytes.data(), 4).has_value());
	}

	TEST_CASE("decode rejects an empty buffer") {
		CHECK_FALSE(DockLayoutSerializer::decode(nullptr, 0).has_value());
	}

	TEST_CASE("apply_layout rebuilds the tree and reuses panels by id") {
		DockSpace space;
		DockNode *root = space.get_root_node();
		REQUIRE(root != nullptr);

		const char *ids[] = { "p1", "p2", "p3", "p4" };
		for (const char *id : ids) {
			DockPanel *panel = root->add_panel(id);
			panel->set_id(id);
		}

		DockLayoutDesc desc;
		desc.root.is_leaf = false;
		desc.root.direction = 1;

		DockNodeDesc top;
		top.is_leaf = false;
		top.direction = 0;
		top.fraction = 0.7F;

		DockNodeDesc left;
		left.is_leaf = true;
		left.fraction = 0.3F;
		left.active_index = 0;
		left.panel_ids = { "p1" };

		DockNodeDesc middle;
		middle.is_leaf = true;
		middle.fraction = 0.7F;
		middle.active_index = 1;
		middle.panel_ids = { "p2", "p3" };

		top.children = { left, middle };

		DockNodeDesc bottom;
		bottom.is_leaf = true;
		bottom.fraction = 0.3F;
		bottom.active_index = 0;
		bottom.panel_ids = { "p4" };

		desc.root.children = { top, bottom };

		REQUIRE(space.apply_layout(desc));

		const DockLayoutDesc after = DockLayoutSerializer::capture(space);
		check_structure(desc.root, after.root);
	}

	TEST_CASE("apply_layout re-homes panels missing from the description") {
		DockSpace space;
		DockNode *root = space.get_root_node();
		REQUIRE(root != nullptr);

		for (const char *id : { "keep", "orphan" }) {
			DockPanel *panel = root->add_panel(id);
			panel->set_id(id);
		}

		DockLayoutDesc desc;
		desc.root.is_leaf = true;
		desc.root.active_index = 0;
		desc.root.panel_ids = { "keep" };

		REQUIRE(space.apply_layout(desc));

		const DockLayoutDesc after = DockLayoutSerializer::capture(space);
		REQUIRE(after.root.is_leaf);
		REQUIRE(after.root.panel_ids.size() == 2);
		CHECK(after.root.panel_ids[0] == "keep");
		CHECK(after.root.panel_ids[1] == "orphan");
	}
}
