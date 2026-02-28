#pragma once

#include "Aquila/Events/Event.h"

namespace Aquila::Events {

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

} // namespace Aquila::Events
