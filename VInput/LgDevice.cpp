#include "pch.h"
#include "LgDevice.h"
#include "StringHelper.h"
#include "Win32Helper.h"

#include <cfgmgr32.h>
#include <devpkey.h>
#include <initguid.h>

#include <SetupAPI.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <cstdint>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>
#include <mutex>

#include "Common.h"

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Cfgmgr32.lib")


namespace VInput::Lg {

	static constexpr DWORD IOCTL_LGHUB_KEYBOARD_REPORT = 0x002A200C;
	static constexpr DWORD IOCTL_LGHUB_MOUSE_REPORT = 0x002A2010;
	static constexpr DWORD IOCTL_LGHUB_JOYSTICK_REPORT = 0x002A203C;

	bool LgDevice::SendBufferedIoctl(DWORD ioctlCode, void* inputBuffer, DWORD inputSize, void* outputBuffer, DWORD outputSize, DWORD* bytesReturned)
	{
		std::scoped_lock lock(mutex_);

		if (deviceHandle_ == INVALID_HANDLE_VALUE)
			return false;

		DWORD localBytesReturned = 0;
		BOOL result = DeviceIoControl(deviceHandle_, ioctlCode, inputBuffer, inputSize, outputBuffer, outputSize, &localBytesReturned, nullptr);

		if (!result)
			return false;

		if (bytesReturned != nullptr)
			*bytesReturned = localBytesReturned;
		return true;
	}

	bool LgDevice::Initialize() {
		if (deviceHandle_ != INVALID_HANDLE_VALUE)
			return true;

		// L"\\\\?\\root#lghubvirtualbus#0000#{1abc05c0-c378-41b9-9cef-df1aba82b015}"
		std::wstring path = VInput::Win32::GetDeviceInterfacePath(GUID_LGHUB_XLCORE_INTERFACE, XLCORE_INTERFACE_PATH);
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

	void LgDevice::Shutdown() noexcept {
		MouseReleaseAll();
		KeyboardReleaseAll();
		if (deviceHandle_ != INVALID_HANDLE_VALUE) {
			CloseHandle(deviceHandle_);
			deviceHandle_ = INVALID_HANDLE_VALUE;
		}
	}

	bool LgDevice::SendMouseReport(const uint8_t buttons, const int8_t dx, const int8_t dy, const int8_t wheel, const int8_t horizontalWheel)
	{
		if (deviceHandle_ == INVALID_HANDLE_VALUE)
			return false;

		if ((buttons == mouseButtons_) && (dx == 0) && (dy == 0) && (wheel == 0) && (horizontalWheel == 0))
			return true;

		std::lock_guard<std::mutex> lock(mutex_);
		MouseInputReport report{ buttons, dx, dy, wheel, horizontalWheel };
		DWORD bytesReturned = 0;
		bool success = DeviceIoControl(deviceHandle_, IOCTL_LGHUB_MOUSE_REPORT, &report, sizeof(report), nullptr, 0, &bytesReturned, nullptr);
		if (success)
			mouseButtons_ = buttons;

		return success;
	}

	bool LgDevice::SendKeyboardReport(const uint8_t modifiers, const uint8_t keys[6])
	{
		static_assert(sizeof(keyboardKeys_) == 6);

		if (deviceHandle_ == INVALID_HANDLE_VALUE)
			return false;

		std::lock_guard<std::mutex> lock(mutex_);
		KeyboardInputReport report{ modifiers, 0, keys[0], keys[1], keys[2], keys[3], keys[4], keys[5] };
		DWORD bytesReturned = 0;
		bool status = DeviceIoControl(deviceHandle_, IOCTL_LGHUB_KEYBOARD_REPORT, &report, sizeof(report), nullptr, 0, &bytesReturned, nullptr);
		if (status) {
			keyboardModifiers_ = modifiers;
			std::copy_n(keys, keyboardKeys_.size(), keyboardKeys_.begin());
		}
		return status;
	}
}
