#ifndef AQUILA_INPUT_EVENTS_H
#define AQUILA_INPUT_EVENTS_H

#include "Aquila/Application/Events/Event.h"

namespace Aquila::Application::Events {

enum class KeyCode : uint16 {
	Unknown = 0,
	Space = 32,
	A = 65,
	B = 66,
	C = 67,
	D = 68,
	E = 69,
	F = 70,
	G = 71,
	H = 72,
	I = 73,
	J = 74,
	K = 75,
	L = 76,
	M = 77,
	N = 78,
	O = 79,
	P = 80,
	Q = 81,
	R = 82,
	S = 83,
	T = 84,
	U = 85,
	V = 86,
	W = 87,
	X = 88,
	Y = 89,
	Z = 90,
	Escape = 256,
	Enter = 1,
	Tab = 258,
	Backspace = 3,
	Delete = 4,
	Right = 5,
	Left = 6,
	Down = 7,
	Up = 8,
	F1 = 9,
	F2 = 10,
	F3 = 11,
	F4 = 12,
	F5 = 13,
	F6 = 14,
	F7 = 15,
	F8 = 16,
	F9 = 17,
	F10 = 18,
	F11 = 19,
	F12 = 20,
	LeftShift = 21,
	LeftControl = 22,
	LeftAlt = 23,
	RightShift = 24,
	RightControl = 25,
	RightAlt = 26
};

enum class MouseButton : uint8 { Left = 0, Right = 1, Middle = 2 };

class KeyPressedEvent final : public Event {
  public:
	KeyPressedEvent(const KeyCode keycode, const int repeatCount) : m_KeyCode(keycode), m_RepeatCount(repeatCount) {}

	[[nodiscard]] KeyCode GetKeyCode() const { return m_KeyCode; }
	[[nodiscard]] int GetRepeatCount() const { return m_RepeatCount; }
	[[nodiscard]] bool IsRepeat() const { return m_RepeatCount > 0; }

	[[nodiscard]] std::string ToString() const override {
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

	[[nodiscard]] KeyCode GetKeyCode() const { return m_KeyCode; }

	[[nodiscard]] std::string ToString() const override {
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

	[[nodiscard]] KeyCode GetKeyCode() const { return m_KeyCode; }

	EVENT_CLASS_TYPE(KeyTypedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Keyboard | EventCategory::Input)

  private:
	KeyCode m_KeyCode;
};

class MouseMovedEvent final : public Event {
  public:
	MouseMovedEvent(const f32 x, const f32 y) : m_MouseX(x), m_MouseY(y) {}

	[[nodiscard]] f32 GetX() const { return m_MouseX; }
	[[nodiscard]] f32 GetY() const { return m_MouseY; }

	[[nodiscard]] std::string ToString() const override {
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

	[[nodiscard]] f32 GetXOffset() const { return m_XOffset; }
	[[nodiscard]] f32 GetYOffset() const { return m_YOffset; }

	[[nodiscard]] std::string ToString() const override {
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

	[[nodiscard]] MouseButton GetMouseButton() const { return m_Button; }

	[[nodiscard]] std::string ToString() const override {
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

	[[nodiscard]] MouseButton GetMouseButton() const { return m_Button; }

	[[nodiscard]] std::string ToString() const override {
		return "MouseButtonReleasedEvent: " + std::to_string(static_cast<int>(m_Button));
	}

	EVENT_CLASS_TYPE(MouseButtonReleasedEvent)
	EVENT_CLASS_CATEGORY(EventCategory::Mouse | EventCategory::Input | EventCategory::MouseButton)

  private:
	MouseButton m_Button;
};

} // namespace Aquila::Application::Events

#endif // AQUILA_INPUT_EVENTS_H
