#ifndef METADATA_COMPONENT_H
#define METADATA_COMPONENT_H

#include "Aquila/Foundation/PrimitiveTypes.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/UUID.h"
namespace Aquila::SceneManagement::Components {
struct MetadataComponent {
  public:
	MetadataComponent() = default;

	MetadataComponent(const Utils::UUID &id, std::string name, bool visible = true, bool selected = false)

		: m_Id(id), m_Name(std::move(name)), m_Visible(visible), m_Selected(selected) {}

	[[nodiscard]] const bool &IsVisible() const { return m_Visible; }
	[[nodiscard]] const bool &IsSelected() const { return m_Selected; }
	[[nodiscard]] const Utils::UUID &GetId() const { return m_Id; }
	[[nodiscard]] const std::string &GetName() const { return m_Name; }

	void SetVisible(bool visible) { m_Visible = visible; }
	void SetSelected(bool selected) { m_Selected = selected; }
	void SetName(const std::string &name) { m_Name = name; }
	void SetId(const Utils::UUID &uuid) { m_Id = uuid; }

  private:
	Utils::UUID m_Id = Utils::UUID::Null();
	std::string m_Name;
	bool m_Visible = true;
	bool m_Selected = false;
};
} // namespace Aquila::SceneManagement::Components
#endif
