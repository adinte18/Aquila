#pragma once

#include "Aquila/Foundation/Signal.h"

namespace Editor {

enum class TransformTool { Translate, Rotate, Scale };
enum class TransformSpace { World, Local };

class EditorTools {
  public:
	void set_tool(TransformTool tool) {
		if (m_tool == tool) {
			return;
		}
		m_tool = tool;
		on_tool_changed(tool);
	}

	void set_space(TransformSpace space) {
		if (m_space == space) {
			return;
		}
		m_space = space;
		on_space_changed(space);
	}

	void toggle_space() { set_space(m_space == TransformSpace::World ? TransformSpace::Local : TransformSpace::World); }

	[[nodiscard]] TransformTool get_tool() const { return m_tool; }
	[[nodiscard]] TransformSpace get_space() const { return m_space; }

	Signal<void(TransformTool)> on_tool_changed;
	Signal<void(TransformSpace)> on_space_changed;

  private:
	TransformTool m_tool = TransformTool::Translate;
	TransformSpace m_space = TransformSpace::World;
};

} // namespace Editor
