#include "pch.h"
#include "UvDevice.h"
#include "StringHelper.h"
#include <setupapi.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <stdio.h>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace VInput::Uv {
	constexpr std::wstring_view UVHID_INTERFACE_PATH = L"\\\\?\\hid#uvhid&col01";

#pragma pack(push, 1)
	struct UvhidOutBuffer {
		UCHAR  VendorReportId;
		UCHAR  DataLength;
		UCHAR  Command;
		UCHAR  ReportId;
		UCHAR  ReportBuffer[61];
	};
#pragma pack(pop)

	bool UvDevice::Initialize()
	{
        GUID hidGuid;
        HidD_GetHidGuid(&hidGuid);

		// \\?\hid#uvhid&col01#1&3b92844c&e&0000#{4d1e55b2-f16f-11cf-88cb-001111000030}
        std::wstring path = VInput::Win32::GetDeviceInterfacePath(hidGuid, UVHID_INTERFACE_PATH);
        if (path.empty())
            return false;

        HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            deviceHandle_ = handle;
            return true;
        }
		return false;
	}

	void UvDevice::Shutdown() noexcept
	{
		MouseReleaseAll();
		KeyboardReleaseAll();
		if (deviceHandle_ != INVALID_HANDLE_VALUE) {
			CloseHandle(deviceHandle_);
			deviceHandle_ = INVALID_HANDLE_VALUE;
		}
	}

	bool UvDevice::SendUvHidReport(uint8_t reportID, void* inputBuffer, uint8_t inputSize)
	{
		constexpr int REPORT_SIZE = 65;
		UvhidOutBuffer report = { 0 };
		report.VendorReportId = 0x40;
		report.DataLength = (uint8_t)(inputSize + 1);
		report.Command = 0x00;
		report.ReportId = reportID;
		memcpy(&report.ReportBuffer, inputBuffer, inputSize);
		DWORD written = 0;
		std::lock_guard<std::mutex> lock(mutex_);
		BOOL ret = WriteFile(deviceHandle_, &report, REPORT_SIZE, &written, nullptr);
		return ret;
	}

	bool UvDevice::SendMouseReport(const uint8_t buttons, const int8_t dx, const int8_t dy, const int8_t wheel, const int8_t horizontalWheel)
	{
		if (deviceHandle_ == INVALID_HANDLE_VALUE)
			return false;

		if ((buttons == mouseButtons_) && (dx == 0) && (dy == 0) && (wheel == 0) && (horizontalWheel == 0))
			return true;

		std::lock_guard<std::mutex> lock(mutex_);
		MouseInputReport report{ buttons, dx, dy, wheel, horizontalWheel };
		DWORD bytesReturned = 0;
		bool success = SendUvHidReport(0x03, &report, sizeof(report));
		if (success)
			mouseButtons_ = buttons;

		return success;
	}

	bool UvDevice::SendKeyboardReport(const uint8_t modifiers, const uint8_t keys[6])
	{
		static_assert(sizeof(keyboardKeys_) == 6);

		if (deviceHandle_ == INVALID_HANDLE_VALUE)
			return false;

		std::lock_guard<std::mutex> lock(mutex_);
		KeyboardInputReport report{ modifiers, 0, keys[0], keys[1], keys[2], keys[3], keys[4], keys[5] };
		DWORD bytesReturned = 0;
		bool status = SendUvHidReport(0x06, &report, sizeof(report));
		if (status) {
			keyboardModifiers_ = modifiers;
			std::copy_n(keys, keyboardKeys_.size(), keyboardKeys_.begin());
		}
		return status;
	}
}

