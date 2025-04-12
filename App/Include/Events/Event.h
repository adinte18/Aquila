//
// Created by alexa on 06/04/2025.
//

#ifndef EVENT_H
#define EVENT_H

#include <string>
#include <utility>

enum class ACTION_TYPE {
    ADD_COMPONENT,
    REMOVE_COMPONENT,
    ADD_ENTITY,
    REMOVE_ENTITY,
    // add more if needed
};

class Event {
public:
    virtual ~Event() = default;

    virtual const char* GetEventType() const = 0;

    bool handled = false;
};

class AddEntityEvent : public Event {
public:
    AddEntityEvent() = default;

    [[nodiscard]] const char* GetEventType() const override {
        return "AddEntityEvent";
    }
};

class RemoveEntityEvent : public Event {
public:
    explicit RemoveEntityEvent(int entityID) : entityID(entityID) {}

    [[nodiscard]] const char* GetEventType() const override {
        return "RemoveEntityEvent";
    }

    int entityID;
};

class AddEnvMapEvent : public Event {
public:
    explicit AddEnvMapEvent(const std::string& texturePath) : texturePath(texturePath) {}

    [[nodiscard]] const std::string& GetTexturePath() const {
        return texturePath;
    }

    [[nodiscard]] const char* GetEventType() const override {
        return "AddEnvMapEvent";
    }
private:
    std::string texturePath;
};


#endif //EVENT_H
