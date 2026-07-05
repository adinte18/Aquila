#ifndef METADATA_COMPONENT_H
#define METADATA_COMPONENT_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/UUID.h"
namespace Aquila::SceneManagement::Components {
struct MetadataComponent {
  public:
	MetadataComponent() = default;

	MetadataComponent(const Foundation::UUID &id, std::string name, bool visible = true, bool selected = false)

		: m_id(id), m_name(std::move(name)), m_visible(visible), m_selected(selected) {}

	[[nodiscard]] const bool &is_visible() const { return m_visible; }
	[[nodiscard]] const bool &is_selected() const { return m_selected; }
	[[nodiscard]] const Foundation::UUID &get_id() const { return m_id; }
	[[nodiscard]] const std::string &get_name() const { return m_name; }

	void set_visible(bool visible) { m_visible = visible; }
	void set_selected(bool selected) { m_selected = selected; }
	void set_name(const std::string &name) { m_name = name; }
	void set_id(const Foundation::UUID &uuid) { m_id = uuid; }

  private:
	Foundation::UUID m_id = Foundation::UUID::null();
	std::string m_name;
	bool m_visible = true;
	bool m_selected = false;
};
} // namespace Aquila::SceneManagement::Components
#endif
