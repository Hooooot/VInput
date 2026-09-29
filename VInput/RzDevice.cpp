#include "pch.h"
#include "RzDevice.h"

#include <ntddkbd.h>
#include <ntddmou.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <array>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

namespace VInput::Rz {
    struct HidScanCodeEntry {
        uint8_t usage; // usage in hid page 07
        USHORT makeCode; // PS2 set1 scancode
        USHORT prefixFlags;
    };

    constexpr std::array kHidScanCodes{
        HidScanCodeEntry{0x04, 0x1E, 0},
        HidScanCodeEntry{0x05, 0x30, 0},
        HidScanCodeEntry{0x06, 0x2E, 0},
        HidScanCodeEntry{0x07, 0x20, 0},
        HidScanCodeEntry{0x08, 0x12, 0},
        HidScanCodeEntry{0x09, 0x21, 0},
        HidScanCodeEntry{0x0A, 0x22, 0},
        HidScanCodeEntry{0x0B, 0x23, 0},
        HidScanCodeEntry{0x0C, 0x17, 0},
        HidScanCodeEntry{0x0D, 0x24, 0},
        HidScanCodeEntry{0x0E, 0x25, 0},
        HidScanCodeEntry{0x0F, 0x26, 0},
        HidScanCodeEntry{0x10, 0x32, 0},
        HidScanCodeEntry{0x11, 0x31, 0},
        HidScanCodeEntry{0x12, 0x18, 0},
        HidScanCodeEntry{0x13, 0x19, 0},
        HidScanCodeEntry{0x14, 0x10, 0},
        HidScanCodeEntry{0x15, 0x13, 0},
        HidScanCodeEntry{0x16, 0x1F, 0},
        HidScanCodeEntry{0x17, 0x14, 0},
        HidScanCodeEntry{0x18, 0x16, 0},
        HidScanCodeEntry{0x19, 0x2F, 0},
        HidScanCodeEntry{0x1A, 0x11, 0},
        HidScanCodeEntry{0x1B, 0x2D, 0},
        HidScanCodeEntry{0x1C, 0x15, 0},
        HidScanCodeEntry{0x1D, 0x2C, 0},

        HidScanCodeEntry{0x1E, 0x02, 0},
        HidScanCodeEntry{0x1F, 0x03, 0},
        HidScanCodeEntry{0x20, 0x04, 0},
        HidScanCodeEntry{0x21, 0x05, 0},
        HidScanCodeEntry{0x22, 0x06, 0},
        HidScanCodeEntry{0x23, 0x07, 0},
        HidScanCodeEntry{0x24, 0x08, 0},
        HidScanCodeEntry{0x25, 0x09, 0},
        HidScanCodeEntry{0x26, 0x0A, 0},
        HidScanCodeEntry{0x27, 0x0B, 0},

        HidScanCodeEntry{0x28, 0x1C, 0},
        HidScanCodeEntry{0x29, 0x01, 0},
        HidScanCodeEntry{0x2A, 0x0E, 0},
        HidScanCodeEntry{0x2B, 0x0F, 0},
        HidScanCodeEntry{0x2C, 0x39, 0},
        HidScanCodeEntry{0x2D, 0x0C, 0},
        HidScanCodeEntry{0x2E, 0x0D, 0},
        HidScanCodeEntry{0x2F, 0x1A, 0},
        HidScanCodeEntry{0x30, 0x1B, 0},
        HidScanCodeEntry{0x31, 0x2B, 0},
        HidScanCodeEntry{0x32, 0x2B, 0},
        HidScanCodeEntry{0x33, 0x27, 0},
        HidScanCodeEntry{0x34, 0x28, 0},
        HidScanCodeEntry{0x35, 0x29, 0},
        HidScanCodeEntry{0x36, 0x33, 0},
        HidScanCodeEntry{0x37, 0x34, 0},
        HidScanCodeEntry{0x38, 0x35, 0},
        HidScanCodeEntry{0x39, 0x3A, 0},

        HidScanCodeEntry{0x3A, 0x3B, 0},
        HidScanCodeEntry{0x3B, 0x3C, 0},
        HidScanCodeEntry{0x3C, 0x3D, 0},
        HidScanCodeEntry{0x3D, 0x3E, 0},
        HidScanCodeEntry{0x3E, 0x3F, 0},
        HidScanCodeEntry{0x3F, 0x40, 0},
        HidScanCodeEntry{0x40, 0x41, 0},
        HidScanCodeEntry{0x41, 0x42, 0},
        HidScanCodeEntry{0x42, 0x43, 0},
        HidScanCodeEntry{0x43, 0x44, 0},
        HidScanCodeEntry{0x44, 0x57, 0},
        HidScanCodeEntry{0x45, 0x58, 0},

        HidScanCodeEntry{0x46, 0x37, KEY_E0},
        HidScanCodeEntry{0x47, 0x46, 0},
        HidScanCodeEntry{0x48, 0x1D, KEY_E1},

        HidScanCodeEntry{0x49, 0x52, KEY_E0},
        HidScanCodeEntry{0x4A, 0x47, KEY_E0},
        HidScanCodeEntry{0x4B, 0x49, KEY_E0},
        HidScanCodeEntry{0x4C, 0x53, KEY_E0},
        HidScanCodeEntry{0x4D, 0x4F, KEY_E0},
        HidScanCodeEntry{0x4E, 0x51, KEY_E0},
        HidScanCodeEntry{0x4F, 0x4D, KEY_E0},
        HidScanCodeEntry{0x50, 0x4B, KEY_E0},
        HidScanCodeEntry{0x51, 0x50, KEY_E0},
        HidScanCodeEntry{0x52, 0x48, KEY_E0},

        HidScanCodeEntry{0x53, 0x45, 0},
        HidScanCodeEntry{0x54, 0x35, KEY_E0},
        HidScanCodeEntry{0x55, 0x37, 0},
        HidScanCodeEntry{0x56, 0x4A, 0},
        HidScanCodeEntry{0x57, 0x4E, 0},
        HidScanCodeEntry{0x58, 0x1C, KEY_E0},
        HidScanCodeEntry{0x59, 0x4F, 0},
        HidScanCodeEntry{0x5A, 0x50, 0},
        HidScanCodeEntry{0x5B, 0x51, 0},
        HidScanCodeEntry{0x5C, 0x4B, 0},
        HidScanCodeEntry{0x5D, 0x4C, 0},
        HidScanCodeEntry{0x5E, 0x4D, 0},
        HidScanCodeEntry{0x5F, 0x47, 0},
        HidScanCodeEntry{0x60, 0x48, 0},
        HidScanCodeEntry{0x61, 0x49, 0},
        HidScanCodeEntry{0x62, 0x52, 0},
        HidScanCodeEntry{0x63, 0x53, 0},
        HidScanCodeEntry{0x64, 0x56, 0},
        HidScanCodeEntry{0x65, 0x5D, KEY_E0},
        HidScanCodeEntry{0x67, 0x59, 0}
    };

    constexpr std::array kModifierScanCodes{
        HidScanCodeEntry{0xE0, 0x1D, 0},
        HidScanCodeEntry{0xE1, 0x2A, 0},
        HidScanCodeEntry{0xE2, 0x38, 0},
        HidScanCodeEntry{0xE3, 0x5B, KEY_E0},
        HidScanCodeEntry{0xE4, 0x1D, KEY_E0},
        HidScanCodeEntry{0xE5, 0x36, 0},
        HidScanCodeEntry{0xE6, 0x38, KEY_E0},
        HidScanCodeEntry{0xE7, 0x5C, KEY_E0}
    };


    constexpr uint32_t IOCTL_RZDEV_INPUT = 0x88883020;


    bool VInput::Rz::RzDevice::Initialize()
    {
        if (deviceHandle_ != INVALID_HANDLE_VALUE)
            return true;

        // L"\\?\RZCONTROL#VID_1532&PID_0306&MI_00#3&1035b5cf&0#{e3be005d-d130-4910-88ff-09ae02f680e9}"
        std::wstring path = VInput::Win32::GetDeviceInterfacePath(GUID_RZ_CONTROL_INTERFACE, RZ_CONTROL_INTERFACE_PATH);
        if (path.empty())
            return false;

        HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle != INVALID_HANDLE_VALUE)
        {
            deviceHandle_ = handle;
            return true;
        }
        return false;
    }

    void VInput::Rz::RzDevice::Shutdown() noexcept
    {
        MouseReleaseAll();
        KeyboardReleaseAll();
        if (deviceHandle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(deviceHandle_);
            deviceHandle_ = INVALID_HANDLE_VALUE;
        }
    }

    bool RzDevice::SendPacketUnlocked(RzInputPacket& packet)
    {
        DWORD bytesReturned = 0;

        return DeviceIoControl(deviceHandle_, IOCTL_RZDEV_INPUT, const_cast<RzInputPacket*>(&packet), static_cast<DWORD>(sizeof(packet)), nullptr, 0, &bytesReturned, nullptr);
    }

    static USHORT MouseButtonTransitions(const uint8_t previousButtons, const uint8_t currentButtons)
    {
        USHORT buttonFlags = 0;
        const uint8_t changed = previousButtons ^ currentButtons;

        if ((changed & kUsbMouseLeft) != 0) {
            buttonFlags |= (currentButtons & kUsbMouseLeft) != 0 ? MOUSE_LEFT_BUTTON_DOWN : MOUSE_LEFT_BUTTON_UP;
        }

        if ((changed & kUsbMouseRight) != 0) {
            buttonFlags |= (currentButtons & kUsbMouseRight) != 0 ? MOUSE_RIGHT_BUTTON_DOWN : MOUSE_RIGHT_BUTTON_UP;
        }

        if ((changed & kUsbMouseMiddle) != 0) {
            buttonFlags |= (currentButtons & kUsbMouseMiddle) != 0 ? MOUSE_MIDDLE_BUTTON_DOWN : MOUSE_MIDDLE_BUTTON_UP;
        }

        if ((changed & kUsbMouseBack) != 0) {
            buttonFlags |= (currentButtons & kUsbMouseBack) != 0 ? MOUSE_BUTTON_4_DOWN : MOUSE_BUTTON_4_UP;
        }

        if ((changed & kUsbMouseForward) != 0) {
            buttonFlags |= (currentButtons & kUsbMouseForward) != 0 ? MOUSE_BUTTON_5_DOWN : MOUSE_BUTTON_5_UP;
        }

        return buttonFlags;
    }

    bool RzDevice::SendMouseReport(const uint8_t buttons, const int8_t dx, const int8_t dy, const int8_t wheel, const int8_t horizontalWheel)
    {
        if (deviceHandle_ == INVALID_HANDLE_VALUE)
            return false;

        if (buttons == mouseButtons_ && dx == 0 && dy == 0 && wheel == 0 && horizontalWheel == 0)
            return true;

        std::lock_guard<std::mutex> lock(mutex_);

        RzInputPacket movementPacket{};
        movementPacket.type = RzInputType::Mouse;
        movementPacket.mouse.Flags = MOUSE_MOVE_RELATIVE;
        movementPacket.mouse.ButtonFlags = MouseButtonTransitions(mouseButtons_, buttons);
        movementPacket.mouse.LastX = dx;
        movementPacket.mouse.LastY = dy;

        if (!SendPacketUnlocked(movementPacket)) {
            return false;
        }

        if (wheel != 0) {
            RzInputPacket wheelPacket{};
            wheelPacket.type = RzInputType::Mouse;
            wheelPacket.mouse.ButtonFlags = MOUSE_WHEEL;
            wheelPacket.mouse.ButtonData = static_cast<USHORT>(static_cast<SHORT>(static_cast<int32_t>(wheel) * WHEEL_DELTA));

            if (!SendPacketUnlocked(wheelPacket)) {
                return false;
            }
        }

        if (horizontalWheel != 0) {
            RzInputPacket wheelPacket{};
            wheelPacket.type = RzInputType::Mouse;
            wheelPacket.mouse.ButtonFlags = MOUSE_HWHEEL;
            wheelPacket.mouse.ButtonData = static_cast<USHORT>(static_cast<SHORT>(static_cast<int32_t>(horizontalWheel) * WHEEL_DELTA));

            if (!SendPacketUnlocked(wheelPacket)) {
                return false;
            }
        }

        mouseButtons_ = buttons;
        return true;
    }

    static std::optional<VInput::Rz::KeyboardCode> HidUsageToKeyboardCode(const uint8_t usage)
    {
        const auto predicate = [&usage](const HidScanCodeEntry& entry) noexcept {
            return entry.usage == usage;
            };

        const auto normalIterator = std::find_if(kHidScanCodes.begin(), kHidScanCodes.end(), predicate);

        if (normalIterator != kHidScanCodes.end()) {
            return KeyboardCode{
                .makeCode = normalIterator->makeCode,
                .prefixFlags = normalIterator->prefixFlags
            };
        }

        const auto modifierIterator = std::find_if(kModifierScanCodes.begin(), kModifierScanCodes.end(), predicate);

        if (modifierIterator != kModifierScanCodes.end()) {
            return KeyboardCode{
                .makeCode = modifierIterator->makeCode,
                .prefixFlags = modifierIterator->prefixFlags
            };
        }

        return std::nullopt;
    }

    static std::optional<VInput::Rz::KeyboardCode> ModifierBitToKeyboardCode(const uint8_t modifierBit)
    {
        if (modifierBit >= 8) {
            return std::nullopt;
        }

        return HidUsageToKeyboardCode(static_cast<uint8_t>(0xE0 + modifierBit));
    }

    static bool ContainsUsage(const std::array<uint8_t, 6>& keys, const uint8_t usage)
    {
        return std::find(keys.begin(), keys.end(), usage) != keys.end();
    }

    bool RzDevice::SendKeyboardEventUnlocked(KeyboardCode& code, const bool pressed)
    {
        RzInputPacket packet{};
        packet.type = RzInputType::Keyboard;
        packet.keyboard.MakeCode = code.makeCode;
        packet.keyboard.Flags = code.prefixFlags;

        if (!pressed) {
            packet.keyboard.Flags |= KEY_BREAK;
        }

        return SendPacketUnlocked(packet);
    }

    bool RzDevice::SendKeyboardReport(const uint8_t modifiers, const uint8_t keys[6])
    {
        static_assert(std::tuple_size_v<decltype(keyboardKeys_)> == 6);

        if (deviceHandle_ == nullptr || deviceHandle_ == INVALID_HANDLE_VALUE) {
            SetLastError(ERROR_INVALID_HANDLE);
            return false;
        }

        if (keys == nullptr) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return false;
        }

        std::array<uint8_t, 6> nextKeys{};
        std::copy_n(keys, nextKeys.size(), nextKeys.begin());

        for (const uint8_t usage : nextKeys) {
            if (usage >= 0x01 && usage <= 0x03) {
                SetLastError(ERROR_INVALID_DATA);
                return false;
            }

            if (usage != 0 && !HidUsageToKeyboardCode(usage).has_value()) {
                SetLastError(ERROR_NOT_SUPPORTED);
                return false;
            }
        }

        std::lock_guard<std::mutex> lock(mutex_);

        if (modifiers == keyboardModifiers_ && nextKeys == keyboardKeys_) {
            return true;
        }

        for (const uint8_t oldUsage : keyboardKeys_) {
            if (oldUsage == 0 || ContainsUsage(nextKeys, oldUsage)) {
                continue;
            }

            std::optional<KeyboardCode> code = HidUsageToKeyboardCode(oldUsage);

            if (!code.has_value() || !SendKeyboardEventUnlocked(*code, false)) {
                return false;
            }
        }

        const uint8_t releasedModifiers = keyboardModifiers_ & static_cast<uint8_t>(~modifiers);

        for (uint8_t bit = 0; bit < 8; ++bit) {
            if ((releasedModifiers & static_cast<uint8_t>(1U << bit)) == 0) {
                continue;
            }

            std::optional<KeyboardCode> code = ModifierBitToKeyboardCode(bit);

            if (!code.has_value() || !SendKeyboardEventUnlocked(*code, false)) {
                return false;
            }
        }

        const uint8_t pressedModifiers = modifiers & static_cast<uint8_t>(~keyboardModifiers_);

        for (uint8_t bit = 0; bit < 8; ++bit) {
            if ((pressedModifiers & static_cast<uint8_t>(1U << bit)) == 0) {
                continue;
            }

            std::optional<KeyboardCode> code = ModifierBitToKeyboardCode(bit);

            if (!code.has_value() || !SendKeyboardEventUnlocked(*code, true)) {
                return false;
            }
        }

        for (const uint8_t newUsage : nextKeys) {
            if (newUsage == 0 || ContainsUsage(keyboardKeys_, newUsage)) {
                continue;
            }

            std::optional<KeyboardCode> code = HidUsageToKeyboardCode(newUsage);

            if (!code.has_value() || !SendKeyboardEventUnlocked(*code, true)) {
                return false;
            }
        }

        keyboardModifiers_ = modifiers;
        keyboardKeys_ = nextKeys;
        return true;
    }
}