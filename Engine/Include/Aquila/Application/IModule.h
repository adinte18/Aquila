#ifndef AQUILA_APPLICATION_I_MODULE_H
#define AQUILA_APPLICATION_I_MODULE_H

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Platform::Events {
class Event;
}

namespace Aquila::Application {

class EngineContext;

class IModule {
  public:
	virtual ~IModule() = default;

	AQUILA_NONCOPYABLE(IModule);
	AQUILA_NONMOVEABLE(IModule);

	virtual void on_attach(EngineContext &engine) {}
	virtual void on_detach() {}

	virtual void on_pre_render(F32 delta_time) {}
	virtual void on_event(Platform::Events::Event &event) {}
	virtual void on_resize(Uint32 width, Uint32 height) {}
	virtual void on_render_resize(Uint32 width, Uint32 height) {}

  protected:
	IModule() = default;
};

}

#endif
