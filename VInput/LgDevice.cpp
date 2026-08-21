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

	DEFINE_GUID(GUID_LGHUB_XLCORE_INTERFACE, 0x1ABC05C0, 0xC378, 0x41B9, 0x9C, 0xEF, 0xDF, 0x1A, 0xBA, 0x82, 0xB0, 0x15);

	static constexpr DWORD IOCTL_LGHUB_KEYBOARD_REPORT = 0x002A200C;
	static constexpr DWORD IOCTL_LGHUB_MOUSE_REPORT = 0x002A2010;
	static constexpr DWORD IOCTL_LGHUB_JOYSTICK_REPORT = 0x002A203C;


#pragma pack(push, 1)
	struct PlugRequest
	{
		std::uint32_t magic;                 // +0x00，调用方设置为 0xB7
		std::uint32_t deviceIdOut;           // +0x04，驱动成功后写回
		std::uint32_t initializedFlag;       // +0x08，驱动覆盖为 1
		std::uint32_t hardwareIdSize;        // +0x0C，单位：字节
		std::uint8_t hardwareId[0x80];       // +0x10..+0x8F, UTF-16 MULTI_SZ，末尾两个 WCHAR NUL
		std::uint16_t productId;             // +0x90，PID
		std::uint16_t vendorId;              // +0x92，VID

		std::uint32_t deviceType;            // +0x94 enum class VirtualDeviceType, 0 = Keyboard, 1 = Mouse, 2 = Other
		std::uint16_t version;               // +0x98

		/*
		 * 驱动会把这三个值当作内核对象指针，并在非零时调用 ObfReferenceObject。
		 * 用户态调用时必须全部保持为 0。
		 */
		std::uint64_t object0;                // +0x9A
		std::uint64_t object1;                // +0xA2
		std::uint64_t object2;                // +0xAA

		std::uint32_t reportDescriptorSize;  // +0xB2，单位：字节
	};

	struct MouseInputReport
	{
		std::uint8_t buttons;
		std::int8_t x;
		std::int8_t y;
		std::int8_t wheel;
		std::int8_t horizontalWheel;
	};

	struct KeyboardInputReport {
		uint8_t modifiers;  // byte 0: LCTRL=1, LSHIFT=2, LALT=4, LWIN=8,
		// RCTRL=16, RSHIFT=32, RALT=64, RWIN=128
		uint8_t reserved;   // byte 1: always 0 (HID spec)
		uint8_t key0;       // bytes 2..7: HID Usage id (a=0x04, ..., f1=0x3A, ...)
		uint8_t key1;		// see HID Keyboard/KeypadPage(0x07)
		uint8_t key2;
		uint8_t key3;
		uint8_t key4;
		uint8_t key5;
	};

#pragma pack(pop)

	enum class MouseButton : std::uint8_t
	{
		None = 0x00,
		Left = 0x01,
		Right = 0x02,
		Middle = 0x04,
		X1 = 0x08,
		X2 = 0x10,
		Button6 = 0x20,
		Button7 = 0x40,
		Button8 = 0x80
	};


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
		const std::vector<std::wstring> paths = VInput::Win32::GetDeviceInterfacePaths(GUID_LGHUB_XLCORE_INTERFACE);
		if (paths.empty())
			return false;

		for (const std::wstring& path : paths) {
			HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (handle != INVALID_HANDLE_VALUE)
			{
				deviceHandle_ = handle;
				return true;
			}
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
