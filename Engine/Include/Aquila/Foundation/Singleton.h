#ifndef SINGLETON_H
#define SINGLETON_H

#include "Aquila/Foundation/Macros.h"
#include "Aquila/Foundation/Defines.h"
#include <string>

namespace Aquila::Foundation {
template <class T> class Singleton {
  public:
	template <typename... Args> static void init(Args &&...args) {
		AQUILA_ASSERT(!s_Instance, (std::string(typeid(T).name()) + ": Singleton already initialized").c_str());
		s_Instance = new T(std::forward<Args>(args)...);
	}

	static T *get() {
		AQUILA_ASSERT(s_Instance, (std::string(typeid(T).name()) + ": Singleton not initialized").c_str());
		return s_Instance;
	}

	static void shutdown() {
		AQUILA_ASSERT(s_Instance, (std::string(typeid(T).name()) + ": Singleton not initialized").c_str());
		delete s_Instance;
		s_Instance = nullptr;
	}

  protected:
	Singleton() = default;
	~Singleton() = default;

  private:
	static inline T *s_Instance = nullptr;
	AQUILA_NONMOVEABLE(Singleton);
	AQUILA_NONCOPYABLE(Singleton);
};
} // namespace Aquila::Foundation

#endif // SINGLETON_H
