#ifndef VIEWPORT_ELEM_H
#define VIEWPORT_ELEM_H

#include "UI/Panels/IPanel.h"

namespace Editor::Panels {
class Viewport : public IPanel {

public:
  void Draw() override;
};
} // namespace Editor::Panels

#endif