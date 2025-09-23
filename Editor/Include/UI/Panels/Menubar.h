#ifndef MENUBAR_COMPONENT_H
#define MENUBAR_COMPONENT_H

#include "UI/Panels/IPanel.h"

namespace Editor {
namespace Panels {
class Menubar : public IPanel {
  bool m_AboutOpened = false;
  bool m_ShowPreferences = false;
  bool m_NewSceneOpened = false;

public:
  void Draw() override;
};
} // namespace Panels
} // namespace Editor

#endif