#ifndef MESH_COMPONENT_H
#define MESH_COMPONENT_H

#include "Engine/Mesh.h"

struct MeshComponent {
    Ref<Engine::Mesh> data{};
};

#endif