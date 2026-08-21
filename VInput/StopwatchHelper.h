#pragma once
#include <windows.h>
#include <chrono>
#include <cstdint>

namespace VInput::Stopwatch {

    class Stopwatch {
    public:
        Stopwatch() noexcept;

        // 开始计时（如果已启动则忽略）
        void Start() noexcept;

        // 停止计时（如果已停止则忽略）
        void Stop() noexcept;

        // 重置计时器，停止并清零
        void Reset() noexcept;

        // 重置并开始计时（相当于 Reset() + Start()）
        void Restart() noexcept;

        // 是否正在运行
        bool IsRunning() const noexcept { return m_isRunning; }

        // 获取从开始到当前（或停止时）的计时周期数
        int64_t ElapsedTicks() const noexcept;

        // 获取毫秒数（double 精度，可容纳较大的时间跨度）
        double ElapsedMilliseconds() const noexcept;

        // 获取微秒数（double 精度）
        double ElapsedMicroseconds() const noexcept;

        // 获取纳秒数（double 精度，注意可能超出精度，仅作参考）
        double ElapsedNanoseconds() const noexcept;

        // 获取 std::chrono::duration 的便捷方法（毫秒）
        std::chrono::milliseconds ElapsedMillisecondsDuration() const noexcept;

    private:
        // 读取当前性能计数器的值
        static int64_t QueryCounter() noexcept;

        // 获取每秒计数器的频率（缓存值）
        static int64_t QueryFrequency() noexcept;

        // 计算已消耗的周期数
        int64_t GetElapsedTicks() const noexcept;

        int64_t m_startTick;      // 开始时的计数
        int64_t m_stopTick;       // 停止时的计数（如果正在运行，则为无效值）
        bool    m_isRunning;      // 当前是否在运行
        static int64_t m_frequency; // 每秒的计数频率（只初始化一次）
    };
}
