//
// Created by alexa on 06/04/2025.
//

#ifndef EVENTSYSTEM_H
#define EVENTSYSTEM_H
#include <functional>


#include "Events/Event.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <typeindex>

namespace Editor {

    class EventDispatcher {
    public:
        template<typename EventType>
        using EventHandler = std::function<void(const EventType&)>;

        template<typename EventType>
        void RegisterHandler(EventHandler<EventType> handler) {
            auto typeIndex = std::type_index(typeid(EventType));
            handlers[typeIndex] = [handler](const Event& event) {
                handler(static_cast<const EventType&>(event));
            };
        }

        void Dispatch(const Event& event) {
            auto typeIndex = std::type_index(typeid(event));
            if (handlers.find(typeIndex) != handlers.end()) {
                handlers[typeIndex](event);
            }
        }

    private:
        // Store a map of event handlers by event type
        std::unordered_map<std::type_index, std::function<void(const Event&)>> handlers;
    };

}

#endif //EVENTSYSTEM_H
