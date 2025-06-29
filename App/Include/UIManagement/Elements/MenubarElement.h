#ifndef MENUBAR_COMPONENT_H
#define MENUBAR_COMPONENT_H

#include "IElement.h"

namespace Editor {
    namespace UIManagement {
        class MenubarElement : public IElement {
            bool m_AboutOpened = false;
            bool m_PreferencesOpened = false;

            public :
            void Draw(void* data) override;
        };
    }
}

#endif