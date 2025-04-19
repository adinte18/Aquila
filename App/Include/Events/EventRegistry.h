//
// Created by alexa on 13/04/2025.
//

#ifndef EVENTREGISTRY_H
#define EVENTREGISTRY_H

#include "Events/EventBus.h"
#include "Scene/Scene.h"

class EventRegistry {
public:
    EventRegistry() = default;

    static void RegisterHandlers() {
        Editor::EventBus::Get().RegisterHandler<UICommandEvent>([](const UICommandEvent& e) {
            switch (e.m_Command) {
                case UICommand::LoadTexture: {
                    auto& path = std::get<std::string>(e.params.at("path"));
                    break;
                }
                case UICommand::AddEntity: {

                }
                default:
                    break;
            }
        });
    }
};

#endif //EVENTREGISTRY_H
