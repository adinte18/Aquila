#ifndef SINGLETON_H
#define SINGLETON_H

#include "Aquila/Foundation/Log.h"
#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Defines.h"

namespace Aquila::Foundation {
template <class T> class Singleton {
  public:
	template <typename... Args>
	static void Init(Args &&...args) {
		AQUILA_ASSERT(!s_Instance, "Singleton already initialized!");
		s_Instance = new T(std::forward<Args>(args)...);
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
} // namespace Aquila::Foundation

#endif // SINGLETON_H
