#ifndef VIEWPORT_ELEM_H
#define VIEWPORT_ELEM_H

#include "Elements/IElement.h"
namespace Editor::UIManagement {
    class ViewportElement : public IElement{
        
        
        public :
        void Draw(void* data) override;
    };
}

#endif