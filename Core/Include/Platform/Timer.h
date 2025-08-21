#ifndef AQUILA_TIMER_H
#define AQUILA_TIMER_H

#include <chrono>

namespace Timer {

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    inline TimePoint Now() {
        return Clock::now();
    }

    inline double ElapsedSeconds(const TimePoint& start, const TimePoint& end) {
        return std::chrono::duration<double>(end - start).count();
    }

    inline double ElapsedMilliseconds(const TimePoint& start, const TimePoint& end) {
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    inline double GetTimeSinceStart(const TimePoint& start) {
        return ElapsedSeconds(start, Now());
    }

    class Stopwatch {
    public:
        Stopwatch() {
            Start();
        }

        void Start() {
            m_StartTime = Now();
            m_LastFrameTime = m_StartTime;
        }

        void Tick() {
            auto currentTime = Now();
            m_DeltaTime = ElapsedSeconds(m_LastFrameTime, currentTime);
            m_LastFrameTime = currentTime;
        }

        float GetDeltaTime() const {
            return m_DeltaTime;
        }

        float GetElapsedTime() const {
            return ElapsedSeconds(m_StartTime, Now());
        }

    private:
        TimePoint   m_StartTime;
        TimePoint   m_LastFrameTime;
        float       m_DeltaTime = 0.0f;
    };


}

#endif // AQUILA_TIMER_H