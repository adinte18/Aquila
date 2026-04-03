#pragma once

#include "Aquila/Application/Events/Event.h"

namespace Aquila::Application::Events {

class AssetSelectedEvent : public Event {
  public:
	explicit AssetSelectedEvent(const std::string &path, const std::string &extension)
		: m_Path(path), m_Extension(extension) {}

	const std::string &GetPath() const { return m_Path; }
	const std::string &GetExtension() const { return m_Extension; }

	EVENT_CLASS_TYPE(AssetSelectedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::AssetSelected)

  private:
	std::string m_Path;
	std::string m_Extension;
};

} // namespace Aquila::Application::Events
