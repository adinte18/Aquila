#include "Engine/Events/EventRegistry.h"
#include "AquilaCore.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/EventBus.h"
#include "Engine/Renderer/Renderer.h"
#include "Platform/Filesystem/VirtualFileSystem.h"
#include "Scene/Components/SceneNodeComponent.h"
#include "Scene/Components/TransformComponent.h"
#include "Scene/Entity.h"
#include "Scene/EntityManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"
#include "Scene/SceneManager.h"

namespace Engine {

bool EventRegistry::s_handlersRegistered = false;

void EventRegistry::RegisterHandlers(Ref<Device> device,
                                     Ref<SceneManager> sceneManager,
                                     Ref<Renderer> renderer) {
  if (s_handlersRegistered) {
    AQUILA_LOG_WARNING("Event handlers already registered!");
    return;
  }

  auto eventBus = EventBus::Get();

  eventBus->RegisterHandler<EntityHasParentQuery>(
      [sceneManager](const EntityHasParentQuery &query) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (query.callback) {
            query.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }

        auto entity = Entity{query.entity, scene};

        auto *node = entity.TryGetComponent<SceneNodeComponent>();
        bool hasParent = node && node->Parent != entt::null;

        if (query.callback) {
          query.callback(EventResult::Success(hasParent));
        }
      });

  eventBus->RegisterHandler<GetExistingEntitiesQuery>(
      [sceneManager](const GetExistingEntitiesQuery &query) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (query.callback) {
            query.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }

        std::vector<Entity> entities;
        auto view = scene->GetRegistry().view<SceneNodeComponent>();
        entities.reserve(view.size());

        for (auto entity : view) {
          entities.emplace_back(Entity{entity, scene});
        }

        if (query.callback) {
          query.callback(EventResult::Success(entities));
        }
      });

  eventBus->RegisterHandler<GetAttachableEntitiesQuery>(
      [sceneManager](const GetAttachableEntitiesQuery &query) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (query.callback) {
            query.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }

        std::vector<Entity> attachableEntities;
        auto &registry = scene->GetRegistry();
        auto *sceneGraph = scene->GetSceneGraph();

        if (!sceneGraph) {
          if (query.callback) {
            query.callback(
                EventResult::Error(EventResult::Status::InternalError,
                                   "Scene graph not available"));
          }
          return;
        }

        for (auto e : registry.view<SceneNodeComponent>()) {
          if (e == query.entity)
            continue;
          if (sceneGraph->IsDescendant(registry, query.entity, e))
            continue; // exclude descendants
          if (sceneGraph->IsDescendant(registry, e, query.entity))
            continue; // exclude ancestors

          attachableEntities.emplace_back(Entity{e, scene});
        }

        if (query.callback) {
          query.callback(EventResult::Success(attachableEntities));
        }
      });

  eventBus->RegisterHandler<ContentBrowserTexturesQuery>(
      [device](const ContentBrowserTexturesQuery &query) {
        try {
          std::vector<Ref<Texture2D>> textures;
          std::vector<std::string> texturePaths = {
              TEXTURES_PATH "/folder.png", TEXTURES_PATH "/aquila-logo.png",
              TEXTURES_PATH "/file.png"};

          textures.reserve(texturePaths.size());
          for (const auto &path : texturePaths) {
            auto texture =
                Texture2D::Builder(*device).setFilepath(path).build();
            if (texture) {
              textures.push_back(texture);
            } else {
              AQUILA_LOG_WARNING("Failed to load texture: {}", path);
            }
          }

          if (query.callback) {
            query.callback(EventResult::Success(textures));
          }
        } catch (const std::exception &e) {
          if (query.callback) {
            query.callback(EventResult::Error(
                EventResult::Status::InternalError,
                "Failed to load textures: " + std::string(e.what())));
          }
        }
      });

  eventBus->RegisterHandler<EntityIsDescendantQuery>(
      [sceneManager](const EntityIsDescendantQuery &query) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (query.callback) {
            query.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }

        auto *sceneGraph = scene->GetSceneGraph();
        if (!sceneGraph) {
          if (query.callback) {
            query.callback(
                EventResult::Error(EventResult::Status::InternalError,
                                   "Scene graph not available"));
          }
          return;
        }

        bool isDescendant = sceneGraph->IsDescendant(
            scene->GetRegistry(), query.ancestor, query.descendant);

        if (query.callback) {
          query.callback(EventResult::Success(isDescendant));
        }
      });

  eventBus->RegisterHandler<ViewportResizedEvent>(
      [renderer](const ViewportResizedEvent &event) {
        try {
          renderer->Resize({event.width, event.height});
          if (event.callback) {
            event.callback(EventResult::Success());
          }
        } catch (const std::exception &e) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::InternalError,
                "Viewport resize failed: " + std::string(e.what())));
          }
        }
      });

  eventBus->RegisterHandler<NewSceneEvent>(
      [sceneManager](const NewSceneEvent &event) {
        try {
          sceneManager->EnqueueScene(CreateUnique<AquilaScene>(event.sceneName),
                                     [sceneManager, event](AquilaScene *scene) {
                                       std::string virtualPath =
                                           "/Assets/" + event.sceneName +
                                           ".aqscene";
                                       if (scene) {
                                         scene->Serialize(virtualPath);
                                       }
                                     });
          sceneManager->RequestSceneChange();

          if (event.callback) {
            event.callback(EventResult::Success());
          }
        } catch (const std::exception &e) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::InternalError,
                "Failed to create scene: " + std::string(e.what())));
          }
        }
      });

  eventBus->RegisterHandler<SaveSceneEvent>([sceneManager](
                                                const SaveSceneEvent &event) {
    auto scene = sceneManager->GetActiveScene();

    if (!scene) {
      if (event.callback) {
        event.callback(EventResult::Error(EventResult::Status::SceneNotActive,
                                          "No active scene to save"));
      }
      return;
    }

    try {
      std::string virtualPath;
      if (event.customPath) {
        virtualPath = *event.customPath;
      } else {
        virtualPath = "/Assets/" + scene->GetSceneName() + ".aqscene";
      }

      scene->Serialize(virtualPath);

      if (event.callback) {
        event.callback(EventResult::Success(virtualPath));
      }
    } catch (const std::exception &e) {
      if (event.callback) {
        event.callback(EventResult::Error(EventResult::Status::InternalError,
                                          "Failed to save scene: " +
                                              std::string(e.what())));
      }
    }
  });

  eventBus->RegisterHandler<OpenSceneEvent>([sceneManager](
                                                const OpenSceneEvent &event) {
    try {
      sceneManager->EnqueueScene(CreateUnique<AquilaScene>(),
                                 [path = event.scenePath](AquilaScene *scene) {
                                   scene->Deserialize(path);
                                 });
      sceneManager->RequestSceneChange();

      if (event.callback) {
        event.callback(EventResult::Success());
      }
    } catch (const std::exception &e) {
      if (event.callback) {
        event.callback(EventResult::Error(EventResult::Status::InternalError,
                                          "Failed to open scene: " +
                                              std::string(e.what())));
      }
    }
  });

  eventBus->RegisterHandler<AttachToEntityEvent>(
      [sceneManager](const AttachToEntityEvent &event) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }

        try {
          scene->GetSceneGraph()->AttachTo(scene->GetRegistry(), event.parent,
                                           event.entity);
          if (event.callback) {
            event.callback(EventResult::Success());
          }
        } catch (const std::exception &e) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::OperationFailed,
                "Failed to attach entity: " + std::string(e.what())));
          }
        }
      });

  eventBus->RegisterHandler<AddEntityEvent>([sceneManager](
                                                const AddEntityEvent &event) {
    auto scene = sceneManager->GetActiveScene();
    auto entityManager = scene->GetEntityManager();
    if (!scene) {
      if (event.callback) {
        event.callback(EventResult::Error(EventResult::Status::SceneNotActive,
                                          "No active scene"));
      }
      return;
    }

    std::string entityName = event.entityName
                                 ? *event.entityName
                                 : entityManager->GetDefaultName(event.preset);

    auto entity = entityManager->AddEntity(entityName);
    if (entity.IsValid()) {
      entityManager->ApplyPreset(entity, event.preset);
      AQUILA_LOG_INFO("Added entity {} with UUID : {}", entity.GetName(),
                      entity.GetUUID().ToString());
    }
  });

  eventBus->RegisterHandler<LoadMeshEvent>([device, sceneManager](
                                               const LoadMeshEvent &event) {
    auto scene = sceneManager->GetActiveScene();

    if (!scene) {
      if (event.callback) {
        event.callback(EventResult::Error(EventResult::Status::SceneNotActive,
                                          "No active scene"));
      }
      return;
    }

    auto extractFilename = [](const std::string &fullPath) -> std::string {
      size_t lastSlash = fullPath.find_last_of("/\\");
      std::string filename = (lastSlash != std::string::npos)
                                 ? fullPath.substr(lastSlash + 1)
                                 : fullPath;
      size_t lastDot = filename.find_last_of('.');
      if (lastDot != std::string::npos) {
        filename = filename.substr(0, lastDot);
      }
      return filename;
    };

    auto generateDebugName =
        [&extractFilename](const std::string &vfsPath) -> std::string {
      std::string meshName = extractFilename(vfsPath);
      std::stringstream ss;
      return meshName + "_" + ss.str();
    };

    std::string debugName = generateDebugName(event.meshPath);

    try {
      Entity targetEntity;

      if (!event.targetEntity) {
        targetEntity = scene->GetEntityManager()->AddEntity(debugName);
      } else {
        auto entity = Entity{*event.targetEntity, scene};

        targetEntity = entity;
      }

      auto *meshData = targetEntity.TryGetComponent<MeshComponent>();

      if (!meshData) {
        meshData = &targetEntity.AddComponent<MeshComponent>();
      }

      meshData->data = CreateRef<Mesh>(*device, debugName);
      meshData->data->Load(event.meshPath);

      if (event.callback) {
        event.callback(EventResult::Success(targetEntity));
      }
    } catch (const std::exception &e) {
      if (event.callback) {
        event.callback(EventResult::Error(EventResult::Status::OperationFailed,
                                          "Failed to load mesh: " +
                                              std::string(e.what())));
      }
    }
  });

  eventBus->RegisterHandler<CreateChildEntityEvent>(
      [sceneManager](const CreateChildEntityEvent &event) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }

        // try {
        //   auto childEntity = scene->GetEntityManager()->AddEntity();
        //   scene->GetSceneGraph()->AddChild(scene->GetRegistry(),
        //                                    event.parentEntity,
        //                                    childEntity.GetHandle());

        //   if (event.callback) {
        //     event.callback(EventResult::Success(childEntity));
        //   }
        // } catch (const std::exception &e) {
        //   if (event.callback) {
        //     event.callback(EventResult::Error(
        //         EventResult::Status::OperationFailed,
        //         "Failed to create child entity: " + std::string(e.what())));
        //   }
        // }
      });

  eventBus->RegisterHandler<DisownEntityEvent>(
      [sceneManager](const DisownEntityEvent &event) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }
        try {
          auto entity = Entity{event.entity, scene};

          auto *node = entity.TryGetComponent<SceneNodeComponent>();
          if (node && node->Parent != entt::null) {
            scene->GetSceneGraph()->RemoveChild(scene->GetRegistry(),
                                                node->Parent, node->Entity);
            if (event.callback) {
              event.callback(EventResult::Success());
            }
          } else {
            if (event.callback) {
              event.callback(
                  EventResult::Error(EventResult::Status::OperationFailed,
                                     "Entity has no parent to disown from"));
            }
          }
        } catch (const std::exception &e) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::OperationFailed,
                "Failed to disown entity: " + std::string(e.what())));
          }
        }
      });

  eventBus->RegisterHandler<DeleteEntityEvent>(
      [sceneManager](const DeleteEntityEvent &event) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }

        scene->GetEntityManager()->QueueForKill(event.entity);
      });

  eventBus->RegisterHandler<AddPrimitiveEvent>(
      [device, sceneManager](const AddPrimitiveEvent &event) {
        auto scene = sceneManager->GetActiveScene();

        if (!scene) {
          if (event.callback) {
            event.callback(EventResult::Error(
                EventResult::Status::SceneNotActive, "No active scene"));
          }
          return;
        }
      });

  s_handlersRegistered = true;
  eventBus->Initialize();
  eventBus->EnableMetrics(true);

  AQUILA_LOG_INFO("Event handlers registered successfully");
}

void EventRegistry::UnregisterHandlers() {
  if (!s_handlersRegistered) {
    return;
  }

  auto eventBus = EventBus::Get();
  eventBus->Clear();
  eventBus->Shutdown();

  s_handlersRegistered = false;
  AQUILA_LOG_INFO("Event handlers unregistered");
}
} // namespace Engine