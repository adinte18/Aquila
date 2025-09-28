#ifndef METADATA_COMPONENT_H
#define METADATA_COMPONENT_H

#include "AquilaCore.h"

struct MetadataComponent {
  Utility::UUID ID;
  std::string Name;
  bool Visible = true;
  bool Selected = false;

  MetadataComponent() = default;

  MetadataComponent(const Utility::UUID &id, const std::string &name,
                    bool visible = true, bool selected = false)
      : ID(id), Name(name), Visible(visible), Selected(selected) {}
};

#endif