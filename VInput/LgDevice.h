#pragma once

#include "pch.h"
#include "Common.h"
#include <cstdint>
#include <mutex>

namespace VInput::Lg {

	DEFINE_GUID(GUID_LGHUB_XLCORE_INTERFACE, 0x1ABC05C0, 0xC378, 0x41B9, 0x9C, 0xEF, 0xDF, 0x1A, 0xBA, 0x82, 0xB0, 0x15);
	constexpr std::wstring_view XLCORE_INTERFACE_PATH = L"\\\\?\\root#lghubvirtualbus";

	class LgDevice : public VInput::AbsDevice {

	public:
		LgDevice() {}
		~LgDevice() {
			if (deviceHandle_ != INVALID_HANDLE_VALUE)
			{
				MouseReleaseAll();
				KeyboardReleaseAll();
				CloseHandle(deviceHandle_);
			}
		}

		// 通过 AbsDevice 继承
		bool Initialize() override;
		void Shutdown() noexcept override;
		bool SendMouseReport(const uint8_t buttons, const int8_t dx, const int8_t dy, const int8_t wheel, const int8_t horizontalWheel) override;
		bool SendKeyboardReport(const uint8_t modifiers, const uint8_t keys[6]) override;

	private:
		bool SendBufferedIoctl(DWORD ioctlCode, void* inputBuffer, DWORD inputSize, void* outputBuffer, DWORD outputSize, DWORD* bytesReturned);
	};
}