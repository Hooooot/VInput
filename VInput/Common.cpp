#include "pch.h"
#include "Common.h"

#include <windows.h>
#include <iostream>
#include <cstdint>
#include "StringHelper.h"
#include <thread>


std::uint8_t ConvertToHidUsage(std::uint8_t vk_key) noexcept
{
    // A-Z -> Keyboard a and A 到 Keyboard z and Z。
    if (vk_key >= 'A' && vk_key <= 'Z')
    {
        return static_cast<std::uint8_t>(0x04 + vk_key - 'A');
    }

    // 主键盘数字键 1-9。
    if (vk_key >= '1' && vk_key <= '9')
    {
        return static_cast<std::uint8_t>(0x1E + vk_key - '1');
    }

    if (vk_key == '0')
    {
        return 0x27;
    }

    // F1-F12。
    if (vk_key >= VK_F1 && vk_key <= VK_F12)
    {
        return static_cast<std::uint8_t>(0x3A + vk_key - VK_F1);
    }

    // F13-F24。
    if (vk_key >= VK_F13 && vk_key <= VK_F24)
    {
        return static_cast<std::uint8_t>(0x68 + vk_key - VK_F13);
    }

    // 数字小键盘 1-9。
    if (vk_key >= VK_NUMPAD1 && vk_key <= VK_NUMPAD9)
    {
        return static_cast<std::uint8_t>(0x59 + vk_key - VK_NUMPAD1);
    }

    switch (vk_key)
    {
    case VK_RETURN:
        return 0x28;

    case VK_ESCAPE:
        return 0x29;

    case VK_BACK:
        return 0x2A;

    case VK_TAB:
        return 0x2B;

    case VK_SPACE:
        return 0x2C;

    case VK_OEM_MINUS:
        return 0x2D;

    case VK_OEM_PLUS:
        return 0x2E;

    case VK_OEM_4:
        return 0x2F;

    case VK_OEM_6:
        return 0x30;

    case VK_OEM_5:
        return 0x31;

    case VK_OEM_1:
        return 0x33;

    case VK_OEM_7:
        return 0x34;

    case VK_OEM_3:
        return 0x35;

    case VK_OEM_COMMA:
        return 0x36;

    case VK_OEM_PERIOD:
        return 0x37;

    case VK_OEM_2:
        return 0x38;

    case VK_CAPITAL:
        return 0x39;

    case VK_SNAPSHOT:
        return 0x46;

    case VK_SCROLL:
        return 0x47;

    case VK_PAUSE:
        return 0x48;

    case VK_INSERT:
        return 0x49;

    case VK_HOME:
        return 0x4A;

    case VK_PRIOR:
        return 0x4B;

    case VK_DELETE:
        return 0x4C;

    case VK_END:
        return 0x4D;

    case VK_NEXT:
        return 0x4E;

    case VK_RIGHT:
        return 0x4F;

    case VK_LEFT:
        return 0x50;

    case VK_DOWN:
        return 0x51;

    case VK_UP:
        return 0x52;

    case VK_NUMLOCK:
        return 0x53;

    case VK_DIVIDE:
        return 0x54;

    case VK_MULTIPLY:
        return 0x55;

    case VK_SUBTRACT:
        return 0x56;

    case VK_ADD:
        return 0x57;

    case VK_NUMPAD0:
        return 0x62;

    case VK_DECIMAL:
        return 0x63;

    case VK_OEM_102:
        return 0x64;

    case VK_APPS:
        return 0x65;

    case VK_SEPARATOR:
        return 0x85;

    default:
        return 0x00;
    }
}

std::uint8_t ConvertToModifier(std::uint8_t vk_key) noexcept
{
    switch (vk_key)
    {
    case VK_LCONTROL:
        return 0x01;

    case VK_LSHIFT:
        return 0x02;

    case VK_LMENU:
        return 0x04;

    case VK_LWIN:
        return 0x08;

    case VK_RCONTROL:
        return 0x10;

    case VK_RSHIFT:
        return 0x20;

    case VK_RMENU:
        return 0x40;

    case VK_RWIN:
        return 0x80;

    default:
        return 0x00;
    }
}

std::uint8_t HidModifierBitFromUsage(std::uint8_t usage) noexcept
{
    if (usage < 0xE0 || usage > 0xE7)
        return 0;

    return static_cast<std::uint8_t>(1U << (usage - 0xE0));
}

static uint8_t ConvertVkToMouseMask(const uint8_t previousButtons, const uint8_t vk, const bool keyDown)
{
    std::uint8_t buttonMask = 0x00;

    switch (vk)
    {
    case VK_LBUTTON:
        buttonMask = kUsbMouseLeft;
        break;

    case VK_RBUTTON:
        buttonMask = kUsbMouseRight;
        break;

    case VK_MBUTTON:
        buttonMask = kUsbMouseMiddle;
        break;

    case VK_XBUTTON1:
        buttonMask = kUsbMouseBack;
        break;

    case VK_XBUTTON2:
        buttonMask = kUsbMouseForward;
        break;

    default:
        buttonMask = 0x00;
        break;
    }
    if (keyDown)
        return previousButtons | buttonMask;
    else
        return previousButtons & (~buttonMask);
}

namespace VInput {

    bool AbsDevice::MouseMove(const int32_t dx, const int32_t dy)
    {
        if (dx == 0 && dy == 0)
            return true;

        const std::int64_t totalX = dx;
        const std::int64_t totalY = dy;
        const std::int64_t absoluteX = totalX < 0 ? -totalX : totalX;
        const std::int64_t absoluteY = totalY < 0 ? -totalY : totalY;
        const std::int64_t stepCount = (std::max(absoluteX, absoluteY) + 126) / 127;
        std::int64_t previousX = 0;
        std::int64_t previousY = 0;

        auto speedInfo = VInput::Win32::SetSafeMouseSpeed();

        for (std::int64_t step = 1; step <= stepCount; ++step)
        {
            const std::int64_t currentX = totalX * step / stepCount;
            const std::int64_t currentY = totalY * step / stepCount;
            const std::int8_t moveX = static_cast<std::int8_t>(currentX - previousX);
            const std::int8_t moveY = static_cast<std::int8_t>(currentY - previousY);

            if (!SendMouseReport(mouseButtons_, moveX, moveY, 0, 0))
            {
                VInput::Win32::RecoveryMouseSpeed(speedInfo);
                return false;
            }

            previousX = currentX;
            previousY = currentY;
        }
        VInput::Win32::RecoveryMouseSpeed(speedInfo);
        return true;
    }

    /// <summary>
    /// Slower speed than MouseMoveTo
    /// </summary>
    /// <param name="x"></param>
    /// <param name="y"></param>
    /// <returns></returns>
    bool AbsDevice::MouseSlideTo(const int32_t x, const int32_t y)
    {
        constexpr int maximumAttempts = 256;
        constexpr std::int64_t tolerance = 2;

        for (int attempt = 0; attempt < maximumAttempts; ++attempt)
        {
            POINT position{};

            if (!GetCursorPos(&position)) {
                return false;
            }
            const std::int64_t remainingX = static_cast<std::int64_t>(x) - position.x;
            const std::int64_t remainingY = static_cast<std::int64_t>(y) - position.y;
            const std::int64_t absoluteX = remainingX < 0 ? -remainingX : remainingX;
            const std::int64_t absoluteY = remainingY < 0 ? -remainingY : remainingY;
            if (absoluteX <= tolerance && absoluteY <= tolerance) {
                return true;
            }
            const std::int64_t distance = std::max(absoluteX, absoluteY);
            const std::int64_t maximumStep = distance > 32 ? 32 : distance > 8 ? 8 : 2;
            std::int64_t moveX = remainingX;
            std::int64_t moveY = remainingY;

            if (distance > maximumStep)
            {
                moveX = remainingX * maximumStep / distance;
                moveY = remainingY * maximumStep / distance;
            }
            if (moveX == 0 && remainingX != 0)
                moveX = remainingX > 0 ? 1 : -1;
            if (moveY == 0 && remainingY != 0)
                moveY = remainingY > 0 ? 1 : -1;

            if (!SendMouseReport(mouseButtons_, static_cast<std::int8_t>(moveX), static_cast<std::int8_t>(moveY), 0, 0))
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        POINT position{};
        if (!GetCursorPos(&position))
            return false;

        const std::int64_t errorX = static_cast<std::int64_t>(x) - position.x;
        const std::int64_t errorY = static_cast<std::int64_t>(y) - position.y;
        const std::int64_t absoluteErrorX = errorX < 0 ? -errorX : errorX;
        const std::int64_t absoluteErrorY = errorY < 0 ? -errorY : errorY;

        return absoluteErrorX <= tolerance && absoluteErrorY <= tolerance;
    }

    bool AbsDevice::MouseMoveTo(const int32_t x, const int32_t y)
    {
        POINT position{};

        if (!GetCursorPos(&position))
            return false;

        const std::int64_t totalX = static_cast<std::int64_t>(x) - position.x;
        const std::int64_t totalY = static_cast<std::int64_t>(y) - position.y;

        if (totalX == 0 && totalY == 0)
            return true;

        const std::int64_t absoluteX = totalX < 0 ? -totalX : totalX;
        const std::int64_t absoluteY = totalY < 0 ? -totalY : totalY;
        const std::int64_t stepCount = (std::max(absoluteX, absoluteY) + 126) / 127;

        std::int64_t previousX = 0;
        std::int64_t previousY = 0;

        auto speedInfo = VInput::Win32::SetSafeMouseSpeed();

        for (std::int64_t step = 1; step <= stepCount; ++step)
        {
            const std::int64_t currentX = totalX * step / stepCount;
            const std::int64_t currentY = totalY * step / stepCount;
            const std::int8_t moveX = static_cast<std::int8_t>(currentX - previousX);
            const std::int8_t moveY = static_cast<std::int8_t>(currentY - previousY);

            if (!SendMouseReport(mouseButtons_, moveX, moveY, 0, 0))
            {
                VInput::Win32::RecoveryMouseSpeed(speedInfo);
                return false;
            }

            previousX = currentX;
            previousY = currentY;
        }
        VInput::Win32::RecoveryMouseSpeed(speedInfo);
        return true;
    }

    bool AbsDevice::MouseClick(const uint8_t vk_button)
    {
        if (!SendMouseReport(ConvertVkToMouseMask(mouseButtons_, vk_button, true), 0, 0, 0, 0))
            return false;

        std::this_thread::sleep_for(std::chrono::milliseconds(35));

        if (!SendMouseReport(ConvertVkToMouseMask(mouseButtons_, vk_button, false), 0, 0, 0, 0))
            return false;

        return true;
    }

    bool AbsDevice::MouseMoveToClick(const int32_t x, const int32_t y, const uint8_t vk_button)
    {
        MouseMoveTo(x, y);
        return MouseClick(vk_button);
    }

    bool AbsDevice::MousePress(const uint8_t vk_button)
    {
        return SendMouseReport(ConvertVkToMouseMask(mouseButtons_, vk_button, true), 0, 0, 0, 0);
    }

    bool AbsDevice::MouseRelease(const uint8_t vk_button)
    {
        return SendMouseReport(ConvertVkToMouseMask(mouseButtons_, vk_button, false), 0, 0, 0, 0);
    }

    bool AbsDevice::MouseWheel(int8_t delta)
    {
        return SendMouseReport(mouseButtons_, 0, 0, delta, 0);
    }

    bool AbsDevice::MouseHorizontalWheel(int8_t horizontalDelta)
    {
        return SendMouseReport(mouseButtons_, 0, 0, 0, horizontalDelta);
    }

    bool AbsDevice::MouseReleaseAll()
    {
        return SendMouseReport(0, 0, 0, 0, 0);
    }

    bool AbsDevice::KeyboardPress(const uint8_t vk_key)
    {
        uint8_t usage = ConvertToHidUsage(vk_key);
        uint8_t mod = ConvertToModifier(vk_key);
        std::array<uint8_t, 6> keys{};
        keys = keyboardKeys_;
        if (mod) {
            mod |= keyboardModifiers_;
            return SendKeyboardReport(mod, keyboardKeys_.data());
        }

        if (usage) {
            for (int i = 0; i < sizeof(keyboardKeys_); ++i) {
                if (keys[i] == usage) {
                    return true;
                }
                else if (keys[i] == 0) {
                    keys[i] = usage;
                    return SendKeyboardReport(keyboardModifiers_, keys.data());
                }
            }
        }
        return false;
    }

    bool AbsDevice::KeyboardRelease(const uint8_t vk_key)
    {
        uint8_t usage = ConvertToHidUsage(vk_key);
        uint8_t mod = ConvertToModifier(vk_key);
        std::array<uint8_t, 6> keys = keyboardKeys_;

        if (mod) {
            mod = (~mod) & keyboardModifiers_;
            return SendKeyboardReport(mod, keyboardKeys_.data());
        }

        if (usage) {
            for (int i = 0; i < sizeof(keyboardKeys_); ++i) {
                if (keys[i] == usage) {
                    keys[i] = 0;
                    return SendKeyboardReport(keyboardModifiers_, keys.data());
                }
            }
            return true;
        }
        return false;
    }

    bool AbsDevice::KeyboardClick(const uint8_t vk_key)
    {
        if (KeyboardPress(vk_key)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(35));
            return KeyboardRelease(vk_key);
        }
        return false;
    }

    bool AbsDevice::KeyboardPressHID(const uint8_t hid_usage)
    {
        uint8_t mod = HidModifierBitFromUsage(hid_usage);

        if (mod != 0) {
            mod |= keyboardModifiers_;
            return SendKeyboardReport(mod, keyboardKeys_.data());
        }

        std::array<uint8_t, 6> keys = keyboardKeys_;
        for (int i = 0; i < sizeof(keyboardKeys_); ++i) {
            if (keys[i] == hid_usage) {
                return true;
            }
            else if (keys[i] == 0) {
                keys[i] = hid_usage;
                return SendKeyboardReport(keyboardModifiers_, keys.data());
            }
        }
        return false;
    }

    bool AbsDevice::KeyboardReleaseHID(const uint8_t hid_usage)
    {
        uint8_t mod = HidModifierBitFromUsage(hid_usage);

        if (mod != 0) {
            mod = (~mod) & keyboardModifiers_;
            return SendKeyboardReport(mod, keyboardKeys_.data());
        }

        std::array<uint8_t, 6> keys = keyboardKeys_;
        for (int i = 0; i < sizeof(keyboardKeys_); ++i) {
            if (keys[i] == hid_usage) {
                keys[i] = 0;
                return SendKeyboardReport(keyboardModifiers_, keys.data());
            }
            return true;
        }
        return false;
    }

    bool AbsDevice::KeyboardClickHID(const uint8_t hid_usage)
    {
        if (KeyboardPressHID(hid_usage)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(35));
            return KeyboardReleaseHID(hid_usage);
        }
        return false;
    }

    bool AbsDevice::KeyboardReleaseAll()
    {
        uint8_t keys[sizeof(keyboardKeys_)] = { 0 };
        return SendKeyboardReport(0, keys);
    }

    VInput::AvailableDriver GetAvailableDriver() {
        std::wstring outBusInf, outHidInf;
        std::wstring outKbdInfPath, outMouInfPath, outVconInfPath, outVkbdInfPath, outVmouInfPath, outVbusInfPath, outCommInfPath;

        auto infos = VInput::Inf::GetDriverPackageInfos([](const auto& info) {
            return VInput::String::ContainsIgnoreCase(info.version.provider, L"Razer") ||
                VInput::String::ContainsIgnoreCase(info.version.provider, L"Logitech");
            });

        auto status = VInput::Inf::GetVenderDriverStatus(infos, {
                {L"logi_joy_bus_enum.inf", &outBusInf},
                {L"logi_joy_vir_hid.inf",  &outHidInf}
            }, L"Logitech");
        if (status != VInput::Win32::DriverErrorStatus::DRIVER_INCOMPATIBLE)
            return AvailableDriver::LOGITECH;

        status = VInput::Inf::GetVenderDriverStatus(infos, {
            { L"RzDevU_0306_Kbd.inf",  &outKbdInfPath },
            { L"RzDevU_0306_Mou.inf",  &outMouInfPath },
            { L"RzDevU_0306_VCon.inf", &outVconInfPath },
            { L"RzDevU_0306_VKbd.inf", &outVkbdInfPath },
            { L"RzDevU_0306_VMou.inf", &outVmouInfPath },
            { L"RzDevU_0306_VBus.inf", &outVbusInfPath },
            { L"RzCommonU.inf",        &outCommInfPath },
            }, L"Razer");
        if (status != VInput::Win32::DriverErrorStatus::DRIVER_INCOMPATIBLE)
            return AvailableDriver::RAZER;

        return AvailableDriver::NONE;
    }
}

