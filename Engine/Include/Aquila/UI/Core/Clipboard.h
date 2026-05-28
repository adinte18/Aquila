#pragma once
#include <functional>
#include <string>

namespace Aquila::UI::Core {

// Thin clipboard abstraction backed by callbacks set at app startup.
// The application layer (which has GLFW access) installs Get/Set callbacks.
class Clipboard {
  public:
	static void Init(std::function<std::string()> getter, std::function<void(const std::string &)> setter) {
		s_Getter = std::move(getter);
		s_Setter = std::move(setter);
	}

	static std::string Get() { return s_Getter ? s_Getter() : std::string{}; }

	static void Set(const std::string &text) {
		if (s_Setter) {
			s_Setter(text);
		}
	}

  private:
	static inline std::function<std::string()> s_Getter;
	static inline std::function<void(const std::string &)> s_Setter;
};

} // namespace Aquila::UI::Core
