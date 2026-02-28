#ifndef AQUILA_INPUT_EVENTS_H
#define AQUILA_INPUT_EVENTS_H

#include "Aquila/Events/Event.h"

namespace Aquila::Events {

// Key codes
enum class KeyCode {
	Unknown = 0,
	Space = 32,
	A = 65,
	B,
	C,
	D,
	E,
	F,
	G,
	H,
	I,
	J,
	K,
	L,
	M,
	N,
	O,
	P,
	Q,
	R,
	S,
	T,
	U,
	V,
	W,
	X,
	Y,
	Z,
	Escape = 256,
	Enter,
	Tab,
	Backspace,
	Delete,
	Right,
	Left,
	Down,
	Up,
	F1,
	F2,
	F3,
	F4,
	F5,
	F6,
	F7,
	F8,
	F9,
	F10,
	F11,
	F12,
	LeftShift,
	LeftControl,
	LeftAlt,
	RightShift,
	RightControl,
	RightAlt
};

enum class MouseButton { Left = 0, Right = 1, Middle = 2 };

class KeyPressedEvent final : public Event {
  public:
	KeyPressedEvent(const KeyCode keycode, const int repeatCount) : m_KeyCode(keycode), m_RepeatCount(repeatCount) {}

	KeyCode GetKeyCode() const { return m_KeyCode; }
	int GetRepeatCount() const { return m_RepeatCount; }
	bool IsRepeat() const { return m_RepeatCount > 0; }

	std::string ToString() const override {
		return std::string("KeyPressedEvent: ") + std::to_string(static_cast<int>(m_KeyCode)) + " (" +
			   std::to_string(m_RepeatCount) + " repeats)";
	}

	EVENT_CLASS_TYPE(KeyPressedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)

  private:
	KeyCode m_KeyCode;
	int m_RepeatCount;
};

class KeyReleasedEvent final : public Event {
  public:
	explicit KeyReleasedEvent(const KeyCode keycode) : m_KeyCode(keycode) {}

	KeyCode GetKeyCode() const { return m_KeyCode; }

	std::string ToString() const override {
		return std::string("KeyReleasedEvent: ") + std::to_string(static_cast<int>(m_KeyCode));
	}

	EVENT_CLASS_TYPE(KeyReleasedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)

  private:
	KeyCode m_KeyCode;
};

class KeyTypedEvent final : public Event {
  public:
	explicit KeyTypedEvent(const KeyCode keycode) : m_KeyCode(keycode) {}

	KeyCode GetKeyCode() const { return m_KeyCode; }

	EVENT_CLASS_TYPE(KeyTypedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)

  private:
	KeyCode m_KeyCode;
};

class MouseMovedEvent final : public Event {
  public:
	MouseMovedEvent(const f32 x, const f32 y) : m_MouseX(x), m_MouseY(y) {}

	f32 GetX() const { return m_MouseX; }
	f32 GetY() const { return m_MouseY; }

	std::string ToString() const override {
		return "MouseMovedEvent: " + std::to_string(m_MouseX) + ", " + std::to_string(m_MouseY);
	}

	EVENT_CLASS_TYPE(MouseMovedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)

  private:
	f32 m_MouseX, m_MouseY;
};

class MouseScrolledEvent final : public Event {
  public:
	MouseScrolledEvent(const f32 xOffset, const f32 yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

	f32 GetXOffset() const { return m_XOffset; }
	f32 GetYOffset() const { return m_YOffset; }

	std::string ToString() const override {
		return "MouseScrolledEvent: " + std::to_string(m_XOffset) + ", " + std::to_string(m_YOffset);
	}

	EVENT_CLASS_TYPE(MouseScrolledEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input)

  private:
	f32 m_XOffset, m_YOffset;
};

class MouseButtonPressedEvent final : public Event {
  public:
	explicit MouseButtonPressedEvent(const MouseButton button) : m_Button(button) {}

	MouseButton GetMouseButton() const { return m_Button; }

	std::string ToString() const override {
		return "MouseButtonPressedEvent: " + std::to_string(static_cast<int>(m_Button));
	}

	EVENT_CLASS_TYPE(MouseButtonPressedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton)

  private:
	MouseButton m_Button;
};

class MouseButtonReleasedEvent final : public Event {
  public:
	explicit MouseButtonReleasedEvent(const MouseButton button) : m_Button(button) {}

	MouseButton GetMouseButton() const { return m_Button; }

	std::string ToString() const override {
		return "MouseButtonReleasedEvent: " + std::to_string(static_cast<int>(m_Button));
	}

	EVENT_CLASS_TYPE(MouseButtonReleasedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton)

  private:
	MouseButton m_Button;
};

} // namespace Aquila::Events

#endif // AQUILA_INPUT_EVENTS_H
