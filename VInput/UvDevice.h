#pragma once
#include "pch.h"
#include "Common.h"
#include <cstdint>
#include <mutex>


namespace VInput::Uv {

	class UvDevice : public VInput::AbsDevice {

	public:
		UvDevice() {}
		~UvDevice() {
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
		bool SendUvHidReport(uint8_t reportID, void* inputBuffer, uint8_t inputSize);
	};
}