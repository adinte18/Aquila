#ifndef SINGLETON_H
#define SINGLETON_H

#include "Aquila/Core/Defines.h"

namespace Aquila::Utils {
template <class T> class Singleton {
  public:
	static void Init() {
		AQUILA_ASSERT(!s_Instance, "Singleton already initialized!");
		s_Instance = new T();
	}

	static T *Get() {
		AQUILA_ASSERT(s_Instance, "Instance should exist before accessing");
		return s_Instance;
	}

	static void Shutdown() {
		AQUILA_ASSERT(s_Instance, "Instance should exist before deleting");
		delete s_Instance;
		s_Instance = nullptr;
	}

  protected:
	Singleton() = default;
	~Singleton() {};

  private:
	static inline T *s_Instance = nullptr;
	AQUILA_NONMOVEABLE(Singleton);
	AQUILA_NONCOPYABLE(Singleton);
};
} // namespace Aquila::Utils

#endif // SINGLETON_H
