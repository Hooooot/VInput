#include "pch.h"
#include "StopwatchHelper.h"

namespace VInput::Stopwatch {

    // 初始化静态成员
    int64_t VInput::Stopwatch::Stopwatch::m_frequency = 0;

    Stopwatch::Stopwatch() noexcept : m_startTick(0), m_stopTick(0), m_isRunning(false) {
        if (m_frequency == 0) {
            QueryFrequency();
        }
    }

    inline int64_t Stopwatch::QueryCounter() noexcept {
        LARGE_INTEGER li;
        QueryPerformanceCounter(&li);
        return li.QuadPart;
    }

    inline int64_t Stopwatch::QueryFrequency() noexcept {
        LARGE_INTEGER li;
        QueryPerformanceFrequency(&li);
        m_frequency = li.QuadPart;
        return m_frequency;
    }

    void Stopwatch::Start() noexcept {
        if (!m_isRunning) {
            m_startTick = QueryCounter();
            m_isRunning = true;
        }
    }

    void Stopwatch::Stop() noexcept {
        if (m_isRunning) {
            m_stopTick = QueryCounter();
            m_isRunning = false;
        }
    }

    void Stopwatch::Reset() noexcept {
        m_startTick = 0;
        m_stopTick = 0;
        m_isRunning = false;
    }

    void Stopwatch::Restart() noexcept {
        Reset();
        Start();
    }

    int64_t Stopwatch::GetElapsedTicks() const noexcept {
        if (m_isRunning) {
            return QueryCounter() - m_startTick;
        }
        else {
            return m_stopTick - m_startTick;
        }
    }

    int64_t Stopwatch::ElapsedTicks() const noexcept {
        return GetElapsedTicks();
    }

    double Stopwatch::ElapsedMilliseconds() const noexcept {
        if (m_frequency == 0) return 0.0;
        return (static_cast<double>(GetElapsedTicks()) / m_frequency) * 1000.0;
    }

    double Stopwatch::ElapsedMicroseconds() const noexcept {
        if (m_frequency == 0) return 0.0;
        return (static_cast<double>(GetElapsedTicks()) / m_frequency) * 1'000'000.0;
    }

    double Stopwatch::ElapsedNanoseconds() const noexcept {
        if (m_frequency == 0) return 0.0;
        return (static_cast<double>(GetElapsedTicks()) / m_frequency) * 1'000'000'000.0;
    }

    std::chrono::milliseconds Stopwatch::ElapsedMillisecondsDuration() const noexcept {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::duration<double, std::milli>(ElapsedMilliseconds())
        );
    }
}
