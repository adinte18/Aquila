//
// Created by alexa on 06/04/2025.
//

#ifndef UICOMPONENT_H
#define UICOMPONENT_H

class UIComponent {
public:
    virtual ~UIComponent() = default;
    virtual void Render() = 0;
};


#endif //UICOMPONENT_H
