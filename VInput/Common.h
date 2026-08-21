#pragma once

#include <cstdint>
#include "Win32Helper.h"
#include "InfHelper.h"
#include <array>
#include <mutex>

extern std::uint8_t ConvertToHidUsage(std::uint8_t vk_key) noexcept;
extern std::uint8_t ConvertToModifier(std::uint8_t vk_key) noexcept;
extern std::uint8_t HidModifierBitFromUsage(std::uint8_t usage) noexcept;

constexpr uint8_t kUsbMouseLeft = 0x01;
constexpr uint8_t kUsbMouseRight = 0x02;
constexpr uint8_t kUsbMouseMiddle = 0x04;
constexpr uint8_t kUsbMouseBack = 0x08;
constexpr uint8_t kUsbMouseForward = 0x10;

namespace VInput {
    enum CurrentUsingVender {
        Unknown,
        Logitech,
        Razer,
    };

    class AbsDevice {
    public:
        virtual bool Initialize() = 0;
        virtual void Shutdown() noexcept = 0;
        virtual ~AbsDevice() = default;

        // 禁止拷贝
        AbsDevice(const AbsDevice&) = delete;
        AbsDevice& operator=(const AbsDevice&) = delete;

        // 允许移动（因为只有指针/引用传递，通常默认即可）
        AbsDevice(AbsDevice&&) = default;
        AbsDevice& operator=(AbsDevice&&) = default;

        bool MousePress(const uint8_t vk_button);
        bool MouseRelease(const uint8_t vk_button);
        bool MouseMove(const int32_t dx, const int32_t dy);
        bool MouseSlideTo(const int32_t x, const int32_t y);
        bool MouseMoveTo(const int32_t x, const int32_t y);
        bool MouseClick(const uint8_t vk_button);
        bool MouseMoveToClick(const int32_t x, const int32_t y, const uint8_t vk_button);
        bool MouseWheel(int8_t wheel);
        bool MouseHorizontalWheel(int8_t horizontalWheel);
        bool MouseReleaseAll();

        bool KeyboardPress(const uint8_t vk_key);
        bool KeyboardRelease(const uint8_t vk_key);
        bool KeyboardClick(const uint8_t vk_key);
        bool KeyboardPressHID(const uint8_t hid_usage);
        bool KeyboardReleaseHID(const uint8_t hid_usage);
        bool KeyboardClickHID(const uint8_t hid_usage);
        bool KeyboardReleaseAll();


        virtual bool SendMouseReport(const uint8_t buttons, const int8_t dx, const int8_t dy, const int8_t wheel, const int8_t horizontalWheel) = 0;
        virtual bool SendKeyboardReport(const uint8_t modifiers, const uint8_t keys[6]) = 0;

    protected:
        AbsDevice() = default;

        std::mutex mutex_;
        HANDLE deviceHandle_ = INVALID_HANDLE_VALUE;
        uint8_t mouseButtons_ = 0;
        uint8_t keyboardModifiers_ = 0;
        std::array<uint8_t, 6> keyboardKeys_{};
    };

    class IDriver {
    protected:
        IDriver() = default;

    public:
        virtual VInput::Win32::DriverErrorStatus GetDriverStatus() = 0;
        virtual int Uninstall() = 0;
        virtual int Install(const char* utf8DriverDirectory) = 0;
        virtual ~IDriver() = default;

        // 禁止拷贝
        IDriver(const IDriver&) = delete;
        IDriver& operator=(const IDriver&) = delete;

        // 允许移动（因为只有指针/引用传递，通常默认即可）
        IDriver(IDriver&&) = default;
        IDriver& operator=(IDriver&&) = default;
    };

    enum AvailableDriver : int
    {
        NONE = 0,
        LOGITECH,
        RAZER,
    };
    VInput::AvailableDriver GetAvailableDriver();
}
