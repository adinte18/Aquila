#ifndef VIEWPORT_ELEM_H
#define VIEWPORT_ELEM_H

#include "UI/Elements/IElement.h"

namespace Editor::Elements {
    class ViewportElement : public IElement{
        
        
        public :
        void Draw() override;
    };
}

#endif