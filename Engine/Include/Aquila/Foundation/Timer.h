#ifndef AQUILA_TIMER_H
#define AQUILA_TIMER_H

#include "Aquila/Foundation/PrimitiveTypes.h"

namespace Aquila::Foundation {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

inline TimePoint Now() {
	return Clock::now();
}

inline double ElapsedSeconds(const TimePoint &start, const TimePoint &end) {
	return std::chrono::duration<double>(end - start).count();
}

inline double ElapsedMilliseconds(const TimePoint &start, const TimePoint &end) {
	return std::chrono::duration<double, std::milli>(end - start).count();
}

inline double GetTimeSinceStart(const TimePoint &start) {
	return ElapsedSeconds(start, Now());
}

class Stopwatch {
  public:
	Stopwatch() { Start(); }

	void Start() {
		m_StartTime = Now();
		m_LastFrameTime = m_StartTime;
		m_DeltaTime = 0.0f;
	}

	void Tick() {
		auto currentTime = Now();
		m_DeltaTime = ElapsedSeconds(m_LastFrameTime, currentTime);
		m_LastFrameTime = currentTime;
	}

	f32 GetDeltaTime() const { return m_DeltaTime; }

	f32 GetElapsedTime() const { return ElapsedSeconds(m_StartTime, Now()); }

  private:
	TimePoint m_StartTime;
	TimePoint m_LastFrameTime;
	f32 m_DeltaTime = 0.0f;
};

} // namespace Aquila::Foundation

#endif // AQUILA_TIMER_H
