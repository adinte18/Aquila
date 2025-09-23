#ifndef METADATA_COMPONENT_H
#define METADATA_COMPONENT_H

#include "AquilaCore.h"

struct MetadataComponent {
  Utility::UUID ID;
  std::string Name;
  bool Enabled = true;

  MetadataComponent() = default;

  MetadataComponent(const Utility::UUID &id, const std::string &name,
                    bool enabled = true)
      : ID(id), Name(name), Enabled(enabled) {}
};

#endif