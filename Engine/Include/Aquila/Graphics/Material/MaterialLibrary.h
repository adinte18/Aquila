#pragma once
#include "Aquila/Graphics/Material/Material.h"

namespace Aquila::Graphics {

// Simple named registry for materials.
// Does not own GFX resources directly — materials own their own GPU objects.
class MaterialLibrary {
  public:
    void           Register(const std::string &name, Ref<Material> material);
    Ref<Material>  Get(const std::string &name) const;
    bool           Has(const std::string &name) const;
    void           Remove(const std::string &name);

    [[nodiscard]] const std::unordered_map<std::string, Ref<Material>> &GetAll() const {
        return m_Materials;
    }

  private:
    std::unordered_map<std::string, Ref<Material>> m_Materials;
};

} // namespace Aquila::Graphics
