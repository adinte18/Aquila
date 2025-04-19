//
// Created by alexa on 06/04/2025.
//

#ifndef EVENTSYSTEM_H
#define EVENTSYSTEM_H
#include <functional>
#include <mutex>
#include <unordered_map>
#include <typeindex>

#include "Events/Event.h"

namespace Editor {

    class EventBus {
    public:
        template<typename EventType>
        using Handler = std::function<void(const EventType&)>;

        template<typename EventType>
        void RegisterHandler(Handler<EventType> handler) {
            std::lock_guard<std::mutex> lock(mutex);
            auto type = std::type_index(typeid(EventType));

            auto wrapper = [handler](const Event& event) {
                handler(static_cast<const EventType&>(event));
            };

            handlers[type].push_back(wrapper);
        }

        void Dispatch(const Event& event) {
            std::lock_guard<std::mutex> lock(mutex);
            auto type = std::type_index(typeid(event));
            auto it = handlers.find(type);
            if (it != handlers.end()) {
                for (auto& fn : it->second) {
                    fn(event);
                }
            }
        }

        static EventBus& Get() {
            static EventBus instance;
            return instance;
        }

    private:
        std::unordered_map<std::type_index, std::vector<std::function<void(const Event&)>>> handlers;
        std::mutex mutex;
    };

}

#endif //EVENTSYSTEM_H
